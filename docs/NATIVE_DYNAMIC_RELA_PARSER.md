Native dynamic RELA parser and restricted loader integration

The native loader now invokes the parser for a narrow ELF64 big-endian PPC64 ET_EXEC PT_DYNAMIC / DT_RELA subset. It locates DT_RELA, DT_RELASZ and DT_RELAENT, maps the relocation table through file-backed PT_LOAD data, and emits bounded relocation records.

Supported: one bounded PT_DYNAMIC with DT_NULL; at most 128 program headers, 4096 dynamic entries and 256 RELA records; R_PPC64_ADDR32 and R_PPC64_REL32; symbol index zero only; aligned 32-bit targets inside writable PT_LOAD memory. The loader stages RAM and guest mappings, applies the supported relocations to staging, and commits only on success.

Not supported: ET_DYN/load bias, symbol and string tables, nonzero symbols/imports, DT_JMPREL, non-RELA relocations, 64-bit guest addresses, or PS3 firmware/TOC/function-descriptor ABI. This is a restricted ELF research path, not a PS3 game loader. Unsupported or malformed dynamic files are rejected without committing staged RAM or mappings.
