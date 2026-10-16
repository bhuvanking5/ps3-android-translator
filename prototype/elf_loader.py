"""Minimal safe parser for big-endian ELF64 PowerPC64 executable headers.

This only validates and describes ELF metadata. It does not load, map, or execute
file contents; PS3 SELF decryption, relocation, and runtime support are separate.
"""
import struct

ELF_HEADER = struct.Struct(">16sHHIQQQIHHHHHH")
PROGRAM_HEADER = struct.Struct(">IIQQQQQQ")
ELF_MAGIC = b"\x7fELF"
ELFCLASS64 = 2
ELFDATA2MSB = 2
EM_PPC64 = 21
PT_LOAD = 1


def inspect_ps3_elf(data):
    """Return basic metadata for a big-endian ELF64 PowerPC64 executable."""
    if not isinstance(data, (bytes, bytearray, memoryview)):
        raise ValueError("ELF input must be bytes")
    data = bytes(data)
    if len(data) < 4 or data[:4] != ELF_MAGIC:
        raise ValueError("Invalid ELF magic")
    if len(data) < ELF_HEADER.size:
        raise ValueError("File is too small for an ELF64 header")

    fields = ELF_HEADER.unpack_from(data)
    ident, e_type, machine, version, entry, phoff, shoff, flags, ehsize, phentsize, phnum, shentsize, shnum, shstrndx = fields

    if ident[:4] != ELF_MAGIC:
        raise ValueError("Invalid ELF magic")
    if ident[4] != ELFCLASS64:
        raise ValueError("Only ELF64 files are supported")
    if ident[5] != ELFDATA2MSB:
        raise ValueError("Only big-endian ELF files are supported")
    if machine != EM_PPC64:
        raise ValueError("ELF machine is not PowerPC64")
    if version != 1:
        raise ValueError("Unsupported ELF version")
    if ehsize < ELF_HEADER.size:
        raise ValueError("Invalid ELF header size")
    if phnum and phentsize < PROGRAM_HEADER.size:
        raise ValueError("Invalid program-header entry size")
    if phoff > len(data) or phnum > (len(data) - phoff) // max(phentsize, 1):
        raise ValueError("Program-header table is truncated")

    segments = []
    for index in range(phnum):
        offset = phoff + index * phentsize
        p_type, p_flags, p_offset, p_vaddr, p_paddr, p_filesz, p_memsz, p_align = PROGRAM_HEADER.unpack_from(data, offset)
        if p_offset > len(data) or p_filesz > len(data) - p_offset:
            raise ValueError(f"Segment {index} extends beyond the file")
        if p_filesz > p_memsz:
            raise ValueError(f"Segment {index} file size exceeds memory size")
        if p_type == PT_LOAD:
            segments.append({
                "file_offset": p_offset,
                "virtual_address": p_vaddr,
                "file_size": p_filesz,
                "memory_size": p_memsz,
                "flags": p_flags,
                "alignment": p_align,
            })

    return {
        "type": e_type,
        "machine": machine,
        "entry_point": entry,
        "program_header_count": phnum,
        "load_segments": segments,
        "is_executable": e_type == 2,
    }
