# Native guest relocation core (first slice)

This helper is the first native-side foundation for applying PPC64 RELA records to
the current prototype's 32-bit guest memory. It does **not** enable dynamic ELF
loading by itself.

## Supported operations

| Relocation | Formula | Result |
| --- | --- | --- |
| `R_PPC64_ADDR32` (1) | `S + A` | Unsigned 32-bit value |
| `R_PPC64_REL32` (26) | `S + A - P` | Signed 32-bit displacement stored in big-endian form |

Targets must be 4-byte aligned and entirely contained in a mapped writable guest
region. The helper stages all computed values and validates the full batch before
writing any bytes. Batches are capped at 256 records to bound stack usage and work.

## Deliberate limitations

- The caller supplies already-resolved symbol values and target guest addresses.
- This helper does not parse `PT_DYNAMIC`, `DT_RELA`, `DT_JMPREL`, symbol tables,
  or string tables.
- It does not calculate ELF load bias or resolve undefined symbols/imports.
- It rejects every relocation type other than the two listed above.
- The existing native ELF loader still rejects `ET_DYN` and `PT_DYNAMIC`; this
  helper is not yet connected to ELF loading.
- PS3 SELF decryption, firmware exports, PPC64 TOC/function descriptors, ABI
  setup, SPU, and RSX remain unsupported.

Next integration step: implement bounded dynamic-table parsing and symbol
resolution, then call this helper only after all target addresses and symbols
have been validated. Do not remove the loader's dynamic-ELF guard until that
complete path has end-to-end tests.
