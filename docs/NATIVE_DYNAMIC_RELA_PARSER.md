Native dynamic RELA parser and restricted loader integration

The native loader invokes the parser for a bounded ELF64 big-endian PPC64 ET_EXEC PT_DYNAMIC / DT_RELA subset. It locates DT_RELA, DT_RELASZ and DT_RELAENT, maps the relocation table through file-backed PT_LOAD data, and emits bounded relocation records.

Supported: one bounded PT_DYNAMIC with DT_NULL; at most 128 program headers, 4096 dynamic entries and 256 RELA records; R_PPC64_ADDR32 and R_PPC64_REL32; aligned 32-bit targets inside writable PT_LOAD memory. Symbol index zero resolves to zero. Nonzero symbol indices may resolve only to bounded, file-backed ELF64 symbol entries whose section index is not SHN_UNDEF and whose st_value fits the current 32-bit guest address model. If a string table is present, symbol names must point to a terminated string within its declared bounds. Relocation records are staged and copied to the caller only after every record validates.

The loader stages RAM and guest mappings, applies supported relocations to staging, and commits only on success.

Not supported: ET_DYN/load bias, undefined symbols/imports, dynamic import callbacks, DT_JMPREL, non-RELA relocations, other relocation types, 64-bit guest addresses, or PS3 firmware/TOC/function-descriptor ABI. Symbol lookup is deliberately bounded and does not prove full ELF ABI compatibility. This is a restricted ELF research path, not a PS3 game loader. Unsupported or malformed dynamic files are rejected without committing staged RAM or mappings.
