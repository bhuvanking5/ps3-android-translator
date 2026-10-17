Native dynamic RELA parser — restricted milestone

Parses a narrow ELF64 big-endian PPC64 ET_EXEC PT_DYNAMIC table, locates DT_RELA, DT_RELASZ and DT_RELAENT, maps the RELA table through file-backed PT_LOAD data, and emits bounded relocation records.

Supported: one bounded PT_DYNAMIC with DT_NULL; at most 128 program headers, 4096 dynamic entries and 256 RELA records; R_PPC64_ADDR32 and R_PPC64_REL32; symbol index zero only; aligned 32-bit targets inside writable PT_LOAD memory.

Not supported: ET_DYN/load bias, symbol and string tables, nonzero symbols/imports, DT_JMPREL, non-RELA relocations, 64-bit guest addresses, or PS3 firmware/TOC/function-descriptor ABI. This parser is not wired into the loader. Keep the dynamic-ELF guard until an atomic end-to-end load-and-relocate path exists.
