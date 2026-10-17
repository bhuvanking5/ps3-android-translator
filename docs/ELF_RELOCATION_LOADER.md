# Host-side bounded ELF relocation loader

`tools/elf_relocation_loader.py` connects the dynamic ELF inspector to the
small PPC64 RELA helper. It validates and maps a restricted ELF64 big-endian
PowerPC64 image into a host-side bytearray, zero-fills PT_LOAD BSS, verifies
the entry is inside an executable segment, checks relocation targets are
inside writable PT_LOAD segments, and applies the supported relocation list
atomically.

Supported relocation types are currently `R_PPC64_ADDR32`,
`R_PPC64_ADDR64`, and `R_PPC64_REL64`. Undefined symbols require an explicit
callback; without one, loading fails rather than guessing an address.

## Scope and limitations

This is a host-side research harness, not connected to the Android/native ELF
loader and not a PS3 executable runtime. It deliberately rejects overlapping
PT_LOAD ranges and unsupported relocation types. It does not resolve actual
PS3 firmware imports, establish TOC/function-descriptor conventions, apply
instruction-field relocations, initialize the PS3 process ABI, decrypt SELF/PKG,
or execute guest code. Passing tests only validates this narrow mapping and
relocation path.
