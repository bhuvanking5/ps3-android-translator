#!/usr/bin/env python3
"""Read-only ELF64 big-endian PowerPC64 dynamic metadata inspector.

Reports bounded PT_DYNAMIC tags and DT_NEEDED library names. It does not apply
relocations, resolve imports, map an executable, decrypt SELF files, or launch code.
"""
from __future__ import annotations

import argparse
from pathlib import Path
import struct

ELF_HEADER_SIZE = 64
PROGRAM_HEADER_SIZE = 56
DYNAMIC_ENTRY_SIZE = 16
PT_LOAD = 1
PT_DYNAMIC = 2
ET_EXEC = 2
ET_DYN = 3
EM_PPC64 = 21

TAG_NAMES = {
    1: "DT_NEEDED",
    2: "DT_PLTRELSZ",
    5: "DT_STRTAB",
    6: "DT_SYMTAB",
    7: "DT_RELA",
    8: "DT_RELASZ",
    9: "DT_RELAENT",
    10: "DT_STRSZ",
    11: "DT_SYMENT",
    20: "DT_PLTREL",
    23: "DT_JMPREL",
    0: "DT_NULL",
}


class ELFInspectionError(ValueError):
    """Malformed or unsupported ELF input."""


def _u16(data: bytes, offset: int) -> int:
    return struct.unpack_from(">H", data, offset)[0]


def _u32(data: bytes, offset: int) -> int:
    return struct.unpack_from(">I", data, offset)[0]


def _u64(data: bytes, offset: int) -> int:
    return struct.unpack_from(">Q", data, offset)[0]


def inspect_dynamic_elf(data: bytes) -> dict:
    """Return safe dynamic-linking metadata for a bounded ELF64/PPC64 byte string."""
    if len(data) < ELF_HEADER_SIZE or data[:4] != b"\x7fELF":
        raise ELFInspectionError("not a complete ELF header")
    if data[4:7] != bytes((2, 2, 1)):
        raise ELFInspectionError("only ELF64 big-endian version-1 files are supported")
    elf_type = _u16(data, 16)
    if elf_type not in (ET_EXEC, ET_DYN) or _u16(data, 18) != EM_PPC64 or _u32(data, 20) != 1:
        raise ELFInspectionError("only ELF64 big-endian PowerPC64 ET_EXEC/ET_DYN is supported")

    phoff = _u64(data, 32)
    phentsize = _u16(data, 54)
    phnum = _u16(data, 56)
    if phentsize < PROGRAM_HEADER_SIZE or phoff > len(data):
        raise ELFInspectionError("invalid program-header table")
    if phnum > (len(data) - phoff) // phentsize:
        raise ELFInspectionError("truncated program-header table")

    loads: list[tuple[int, int, int, int]] = []
    dynamic: tuple[int, int] | None = None
    for index in range(phnum):
        off = phoff + index * phentsize
        p_type = _u32(data, off)
        p_offset = _u64(data, off + 8)
        p_vaddr = _u64(data, off + 16)
        p_filesz = _u64(data, off + 32)
        p_memsz = _u64(data, off + 40)
        if p_offset > len(data) or p_filesz > len(data) - p_offset:
            raise ELFInspectionError("program segment extends beyond file")
        if p_type == PT_LOAD:
            if p_filesz > p_memsz:
                raise ELFInspectionError("PT_LOAD file size exceeds memory size")
            loads.append((p_offset, p_vaddr, p_filesz, p_memsz))
        elif p_type == PT_DYNAMIC:
            if dynamic is not None:
                raise ELFInspectionError("multiple PT_DYNAMIC segments are not supported")
            dynamic = (p_offset, p_filesz)

    result = {
        "elf_type": "ET_EXEC" if elf_type == ET_EXEC else "ET_DYN",
        "machine": "EM_PPC64",
        "has_dynamic_segment": dynamic is not None,
        "tags": [],
        "needed_libraries": [],
        "relocation_metadata": {},
        "relocations": {"DT_RELA": [], "DT_JMPREL": []},
        "limitations": "Inspection only: relocation entries are decoded but never applied; imports are not resolved and code is not executed.",
    }
    if dynamic is None:
        return result

    dyn_offset, dyn_size = dynamic
    if dyn_size == 0 or dyn_size % DYNAMIC_ENTRY_SIZE:
        raise ELFInspectionError("PT_DYNAMIC size must contain whole 16-byte entries")

    entries: list[tuple[int, int]] = []
    found_null = False
    for off in range(dyn_offset, dyn_offset + dyn_size, DYNAMIC_ENTRY_SIZE):
        tag = struct.unpack_from(">q", data, off)[0]
        value = _u64(data, off + 8)
        if tag == 0:
            found_null = True
            break
        entries.append((tag, value))
    if not found_null:
        raise ELFInspectionError("PT_DYNAMIC has no DT_NULL terminator")

    values_by_tag: dict[int, list[int]] = {}
    for tag, value in entries:
        values_by_tag.setdefault(tag, []).append(value)
        result["tags"].append({
            "tag": TAG_NAMES.get(tag, f"DT_UNKNOWN_{tag}"),
            "value": value,
        })

    def virtual_to_file(address: int, size: int) -> int:
        for file_off, vaddr, filesz, _memsz in loads:
            if address >= vaddr and address - vaddr <= filesz and size <= filesz - (address - vaddr):
                return file_off + (address - vaddr)
        raise ELFInspectionError("dynamic pointer does not map to file-backed PT_LOAD bytes")

    strtab = values_by_tag.get(5, [None])[0]
    strsz = values_by_tag.get(10, [None])[0]
    needed_offsets = values_by_tag.get(1, [])
    if needed_offsets:
        if strtab is None or strsz is None:
            raise ELFInspectionError("DT_NEEDED entries require DT_STRTAB and DT_STRSZ")
        strtab_offset = virtual_to_file(strtab, strsz)
        string_table = data[strtab_offset:strtab_offset + strsz]
        for string_offset in needed_offsets:
            if string_offset >= len(string_table):
                raise ELFInspectionError("DT_NEEDED string offset is outside DT_STRTAB")
            end = string_table.find(b"\0", string_offset)
            if end < 0:
                raise ELFInspectionError("unterminated DT_NEEDED string")
            result["needed_libraries"].append(
                string_table[string_offset:end].decode("utf-8", errors="replace")
            )

    for tag, name in ((7, "DT_RELA"), (8, "DT_RELASZ"), (9, "DT_RELAENT"),
                      (23, "DT_JMPREL"), (2, "DT_PLTRELSZ"), (20, "DT_PLTREL")):
        if tag in values_by_tag:
            result["relocation_metadata"][name] = values_by_tag[tag][0]

    def read_rela_table(pointer_tag: int, size_tag: int, output_key: str) -> list[dict]:
        pointer_values = values_by_tag.get(pointer_tag, [])
        size_values = values_by_tag.get(size_tag, [])
        if not pointer_values and not size_values:
            return []
        if not pointer_values or not size_values:
            raise ELFInspectionError(f"{TAG_NAMES[pointer_tag]} and {TAG_NAMES[size_tag]} must appear together")
        if pointer_tag == 23 and values_by_tag.get(20, [None])[0] != 7:
            raise ELFInspectionError("DT_JMPREL currently supports DT_PLTREL=DT_RELA only")
        entry_size = values_by_tag.get(9, [None])[0]
        if entry_size != 24:
            raise ELFInspectionError("ELF64 RELA entry size must be 24 bytes (DT_RELAENT=24)")
        table_size = size_values[0]
        if table_size % 24:
            raise ELFInspectionError("RELA table size is not a multiple of 24 bytes")
        table_offset = virtual_to_file(pointer_values[0], table_size)
        decoded = []
        for rel_off in range(table_offset, table_offset + table_size, 24):
            target = _u64(data, rel_off)
            info = _u64(data, rel_off + 8)
            addend = struct.unpack_from(">q", data, rel_off + 16)[0]
            decoded.append({
                "offset": target,
                "symbol_index": info >> 32,
                "type": info & 0xFFFFFFFF,
                "addend": addend,
            })
        return decoded

    result["relocations"]["DT_RELA"] = read_rela_table(7, 8, "DT_RELA")
    result["relocations"]["DT_JMPREL"] = read_rela_table(23, 2, "DT_JMPREL")
    return result


def inspect_path(path: Path) -> dict:
    return inspect_dynamic_elf(path.read_bytes())


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path, help="ELF file to inspect")
    args = parser.parse_args()
    try:
        result = inspect_path(args.file)
    except (OSError, ELFInspectionError) as exc:
        parser.error(str(exc))
    print(f"Type: {result['elf_type']} ({result['machine']})")
    print(f"PT_DYNAMIC: {'present' if result['has_dynamic_segment'] else 'absent'}")
    for library in result["needed_libraries"]:
        print(f"Needed library: {library}")
    for tag in result["tags"]:
        print(f"{tag['tag']}: 0x{tag['value']:x}")
    print(result["limitations"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
