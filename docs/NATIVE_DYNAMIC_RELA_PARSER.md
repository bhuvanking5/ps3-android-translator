Native dynamic RELA parser and restricted loader integration

The native loader invokes a bounded ELF64 big-endian PPC64 ET_EXEC PT_DYNAMIC / DT_RELA parser. It locates DT_RELA, DT_RELASZ and DT_RELAENT, maps the relocation table through file-backed PT_LOAD data, and stages relocation records until every record validates.

Supported: one bounded PT_DYNAMIC with DT_NULL; at most 128 program headers, 4096 dynamic entries and 256 RELA records; R_PPC64_ADDR32 and R_PPC64_REL32; aligned 32-bit targets inside writable PT_LOAD memory. Symbol index zero resolves to zero. Nonzero symbols are read from bounded ELF64 DT_SYMTAB entries. Defined symbols are accepted only if their section index is not SHN_UNDEF and their st_value fits the 32-bit guest model. Undefined symbols require both a bounded, terminated name in DT_STRTAB and an explicit caller-supplied resolver callback. The callback must whitelist allowed names, return success only for approved imports, and supply a 32-bit guest address/value. Unknown imports are rejected.

The loader stages RAM and guest mappings, applies supported relocations to staging, and commits only on success. The legacy load function supplies no resolver and therefore rejects undefined imports by default; callers must deliberately use the resolver-enabled API to approve them.

Not supported: ET_DYN/load bias, DT_JMPREL, other relocation types, 64-bit guest addresses, PS3 SELF decryption, PS3 firmware import tables, TOC/function-descriptor ABI, or full PS3 process setup. This is a restricted ELF research path, not a PS3 game loader.
