# Register spill design: PowerPC r31

## Problem

PowerPC exposes 32 general-purpose registers, r0 through r31. AArch64 has 31 general-purpose register names, x0 through x30; encoding 31 denotes the context-dependent stack pointer or zero register, not an ordinary GPR. Therefore a direct mapping of PowerPC r31 to AArch64 x31 is invalid.

The current prototype uses the label `spill_r31` as a policy marker and rejects r31 in the simple assembly pipeline. The label is **not** a working spill slot.

## Minimum safe design

1. Define an explicit runtime frame layout with a reserved, aligned memory slot for guest r31.
2. Define how the frame base is obtained and kept alive across translated instructions.
3. Add code-generation helpers to load r31 into a temporary AArch64 register before an instruction reads it.
4. Store the destination back to the spill slot after an instruction writes r31.
5. Handle instructions where r31 is both a source and destination, ensuring all source values are loaded before the destination store.
6. Reserve scratch registers and specify how they interact with calls, branches, and future register allocation.
7. Keep guest r0 semantics instruction-specific: for example, PowerPC `addi` treats RA=0 as literal zero; other instructions may not share that rule.
8. Add tests for read, write, read-modify-write, aliasing, and block-boundary behavior.

## Correctness requirements

- Never emit x31 as if it were an ordinary GPR.
- Spill/load sequences must preserve the guest-visible 64-bit value.
- The spill address must be valid and aligned for the chosen runtime.
- Branches must preserve the same spill-frame base and guest state.
- Tests must check generated instruction encodings and compare execution against the reference interpreter.

## Implementation boundary

Do not enable r31 translation merely because the register map returns a spill-slot label. Enable it only after a real frame layout, load/store encoding, scratch-register policy, and tests exist.
