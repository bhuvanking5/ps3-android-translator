"""Bounded host-side ELF64/PPC64 load-and-relocate experiment.

This is not the Android/native loader and does not implement the PS3 ABI.
Only non-overlapping PT_LOAD segments and three PPC64 RELA types are accepted.
"""
from __future__ import annotations

import struct
from typing import Callable

from tools.inspect_elf_dynamic import ELFInspectionError, inspect_dynamic_elf
from tools.ppc64_relocations import (
    R_PPC64_ADDR32,
    R_PPC64_ADDR64,
    R_PPC64_REL64,
    RelocationError,
    apply_rela_records,
)

ELF_HEADER_SIZE = 64
PROGRAM_HEADER_SIZE = 56
PT_LOAD = 1
PF_X = 1
PF_W = 2
ET_EXEC = 2
ET_DYN = 3
MAX_IMAGE_BYTES = 64 * 1024 * 1024
MAX_SEGMENTS = 128
MAX_RELOCATIONS = 1_000_000


class ELFLoadError(ValueError):
    """ELF could not be safely mapped by this restricted loader."""


def load_and_relocate(
    data: bytes,
    *,
    load_bias: int = 0,
    resolve_import: Callable | None = None,
    max_image_bytes: int = MAX_IMAGE_BYTES,
) -> dict:
    """Map a bounded ELF image, apply supported RELA entries, and return metadata.

    The returned memory is a host-side bytearray, not executable guest memory.
    Relocation targets must lie wholly inside a writable PT_LOAD segment.
    Undefined symbols require an explicit resolver callback. All relocation
    writes are atomic: any unsupported or invalid relocation rejects the image.
    """
    if len(data) < ELF_HEADER_SIZE or data[:4] != b"\x7fELF":
        raise ELFLoadError("not a complete ELF header")
    if data[4:7] != bytes((2, 2, 1)):
        raise ELFLoadError("only ELF64 big-endian version-1 files are supported")
    elf_type = struct.unpack_from(">H", data, 16)[0]
    if elf_type not in (ET_EXEC, ET_DYN):
        raise ELFLoadError("only ET_EXEC and ET_DYN are supported")
    if not isinstance(load_bias, int) or load_bias < 0:
        raise ELFLoadError("load bias must be non-negative")
    if elf_type == ET_EXEC and load_bias:
        raise ELFLoadError("ET_EXEC must use zero load bias")
    if not isinstance(max_image_bytes, int) or max_image_bytes <= 0 or max_image_bytes > MAX_IMAGE_BYTES:
        raise ELFLoadError("image-size limit must be between 1 byte and 64 MiB")

    try:
        dynamic_info = inspect_dynamic_elf(data)
    except ELFInspectionError as exc:
        raise ELFLoadError(str(exc)) from exc

    entry_vaddr = struct.unpack_from(">Q", data, 24)[0]
    phoff = struct.unpack_from(">Q", data, 32)[0]
    phentsize = struct.unpack_from(">H", data, 54)[0]
    phnum = struct.unpack_from(">H", data, 56)[0]
    if phentsize < PROGRAM_HEADER_SIZE or phoff > len(data):
        raise ELFLoadError("invalid program-header table")
    if phnum > MAX_SEGMENTS or phnum > (len(data) - phoff) // phentsize:
        raise ELFLoadError("program-header table exceeds safe limits")

    segments = []
    for index in range(phnum):
        off = phoff + index * phentsize
        p_type = struct.unpack_from(">I", data, off)[0]
        if p_type != PT_LOAD:
            continue
        flags = struct.unpack_from(">I", data, off + 4)[0]
        file_off, vaddr, filesz, memsz, align = struct.unpack_from(">QQQQQ", data, off + 8)
        if filesz > memsz:
            raise ELFLoadError("PT_LOAD file size exceeds memory size")
        if file_off > len(data) or filesz > len(data) - file_off:
            raise ELFLoadError("PT_LOAD extends beyond file")
        if vaddr > (1 << 64) - 1 - memsz:
            raise ELFLoadError("PT_LOAD virtual-address range overflows")
        if memsz:
            segments.append({
                "vaddr": vaddr, "end": vaddr + memsz, "file_offset": file_off,
                "filesz": filesz, "memsz": memsz, "flags": flags, "align": align,
            })
    if not segments:
        raise ELFLoadError("no non-empty PT_LOAD segments")
    segments.sort(key=lambda segment: segment["vaddr"])
    for previous, current in zip(segments, segments[1:]):
        if current["vaddr"] < previous["end"]:
            raise ELFLoadError("overlapping PT_LOAD memory ranges are not supported")

    min_vaddr = segments[0]["vaddr"]
    max_vaddr = max(segment["end"] for segment in segments)
    span = max_vaddr - min_vaddr
    if span <= 0 or span > max_image_bytes:
        raise ELFLoadError("mapped image exceeds configured memory limit")
    if min_vaddr > (1 << 64) - 1 - load_bias or max_vaddr > (1 << 64) - 1 - load_bias:
        raise ELFLoadError("load-biased address range overflows")
    memory_base = min_vaddr + (load_bias if elf_type == ET_DYN else 0)
    memory = bytearray(span)
    for segment in segments:
        start = segment["vaddr"] - min_vaddr
        memory[start:start + segment["filesz"]] = data[
            segment["file_offset"]:segment["file_offset"] + segment["filesz"]
        ]
        # bytearray initialization already zero-fills the PT_LOAD BSS tail.

    entry = entry_vaddr + (load_bias if elf_type == ET_DYN else 0)
    if entry > (1 << 64) - 1:
        raise ELFLoadError("entry address overflows after load bias")
    if not any(
        segment["vaddr"] <= entry_vaddr < segment["end"] and segment["flags"] & PF_X
        for segment in segments
    ):
        raise ELFLoadError("entry point is not inside an executable PT_LOAD segment")

    relocations = dynamic_info["relocations"]["DT_RELA"] + dynamic_info["relocations"]["DT_JMPREL"]
    if len(relocations) > MAX_RELOCATIONS:
        raise ELFLoadError("relocation count exceeds safe limit")
    width_by_type = {R_PPC64_ADDR32: 4, R_PPC64_ADDR64: 8, R_PPC64_REL64: 8}
    for relocation in relocations:
        width = width_by_type.get(relocation["type"])
        if width is None:
            raise ELFLoadError(f"unsupported PPC64 relocation type: {relocation['type']}")
        target = relocation["offset"]
        if target > (1 << 64) - 1 - (load_bias if elf_type == ET_DYN else 0):
            raise ELFLoadError("relocation target address overflows")
        target_runtime = target + (load_bias if elf_type == ET_DYN else 0)
        if not any(
            segment["vaddr"] <= target and target + width <= segment["end"] and segment["flags"] & PF_W
            for segment in segments
        ):
            raise ELFLoadError("relocation target is not wholly inside a writable PT_LOAD segment")
        if target_runtime < memory_base or target_runtime - memory_base > len(memory) - width:
            raise ELFLoadError("relocation target falls outside mapped image")

    symbol_records = dynamic_info.get("referenced_symbols", {})
    normalized_symbols = {
        index: {
            **symbol,
            "undefined_import": symbol.get("undefined_import", False),
            "section_index": symbol.get("section_index", 0),
        }
        for index, symbol in symbol_records.items()
    }
    try:
        applied = apply_rela_records(
            memory,
            memory_base,
            relocations,
            normalized_symbols,
            elf_type="ET_EXEC" if elf_type == ET_EXEC else "ET_DYN",
            load_bias=load_bias,
            resolve_import=resolve_import,
        )
    except RelocationError as exc:
        raise ELFLoadError(str(exc)) from exc

    return {
        "elf_type": dynamic_info["elf_type"],
        "entry": entry,
        "memory_base": memory_base,
        "memory": memory,
        "mapped_size": len(memory),
        "segments": [
            {
                "address": segment["vaddr"] + (load_bias if elf_type == ET_DYN else 0),
                "size": segment["memsz"],
                "flags": segment["flags"],
            }
            for segment in segments
        ],
        "relocations_applied": applied,
        "needed_libraries": dynamic_info["needed_libraries"],
        "unresolved_imports": [
            symbol["name"] for symbol in normalized_symbols.values()
            if symbol.get("undefined_import")
        ],
        "limitations": (
            "Host-side mapping experiment only; no Android loader integration, "
            "PS3 ABI/TOC setup, firmware services, SELF/PKG support, or game execution."
        ),
    }
