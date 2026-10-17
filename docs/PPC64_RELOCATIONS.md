# Bounded PPC64 relocation helper

This prototype helper applies only three ELF64 PowerPC64 RELA forms to a
caller-provided bytearray, writing values in big-endian order:

- `R_PPC64_ADDR32` (1): `S + A`, unsigned 32-bit.
- `R_PPC64_ADDR64` (38): `S + A`, unsigned 64-bit.
- `R_PPC64_REL64` (44): `S + A - P`, signed 64-bit.

The helper validates target bounds and result ranges before modifying memory.
Unsupported relocation types fail explicitly. The caller must provide an
already-resolved symbol address. Undefined imports must not be passed as if
they were resolved.

## Critical limitations

This helper is not integrated into the Android ELF loader and is not a PS3
dynamic linker. It does not calculate load bias, resolve imports, implement
TOC/function-descriptor conventions, support the broad PPC64 relocation set,
or handle SELF/PKG. Passing unit tests would demonstrate only these isolated
arithmetic/write semantics, not executable or game compatibility.


## Relocation-list stage

The helper now also accepts decoded RELA records plus a caller-provided symbol
table. It applies ET_EXEC records with zero load bias or ET_DYN records with an
explicit load bias, and stages the entire list in a copy before committing.
An undefined symbol requires an explicit resolver callback; without one the
operation fails instead of inventing an address. This is still a host-side
helper and is not wired into the native/Android loader. The callback is only a
mechanism for future testing, not a PS3 firmware import resolver.
