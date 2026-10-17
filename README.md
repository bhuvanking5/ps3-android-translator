# PS3 Android Translator (research prototype)

Experimental research project exploring translation of a **small subset of PowerPC instructions to ARM64** as one possible building block for Android.

> **Status:** early prototype, not a PS3 emulator and not capable of running PS3 games.

## What exists today

- A small PowerPC instruction decoder and reference interpreter, including logical operations and bounded execution for a tiny instruction subset.
- A defensive PS3-style ELF64/PowerPC64 metadata inspector plus a native bounded PT_LOAD loader that copies an unencrypted ELF into caller-owned guest RAM, zero-fills BSS, applies read/write/execute permissions, and validates that the entry point lies in an executable segment. This is not a complete SELF/ELF game loader.
- Experimental Python instruction encoding and translation tools for a limited instruction subset.
- A bounded C PowerPC-to-ARM64 translator for ADDI, ADD, SUBF, MULLD, OR, AND, and XOR. It stores all 32 PowerPC GPRs in a stack-backed frame and uses ARM64 scratch registers for loads, operations, and stores.
- An optional CPython extension API, `ps3_native.translate_and_execute_ppc(words, result_gpr)`, that accepts PowerPC instruction words, validates and translates the supported subset, executes the generated ARM64 on AArch64, and compares the result with the reference interpreter.
- A Python wrapper at `prototype/native_execution.py` for calling that optional native API.
- An Android JNI smoke-test path that translates a fixed built-in PowerPC arithmetic program, executes the generated ARM64 words, and compares the result against the reference interpreter. It never accepts executable bytes from a selected game file.
- A virtual controller UI publishes button bits and normalized analog-stick axes into a native input mailbox, with host-side C tests for input range handling. This is an integration seam only; no PS3 emulation loop consumes it yet. The native adapter also exposes a generic report with button flags and 8-bit axes (0/128/255 = negative/center/positive), covered by host tests. A stable five-word adapter exposes button flags and each axis as zero-extended 32-bit values. A read-only guest-I/O adapter maps those fields at a synthetic prototype address range and rejects unaligned/out-of-range reads; these are test addresses, not real PS3 hardware registers. The bounded PowerPC interpreter supports callback-backed `lwz` and `stw` memory operations. A new bounded guest-memory module maps non-overlapping regions, enforces read/write permissions, rejects invalid or unaligned accesses, and reads/writes big-endian 32-bit words. Tests execute a tiny PowerPC program that stores a value to guest RAM and loads it back. This is a foundation for a future ELF loader, not yet a PS3 memory map or game loader.
- CMake tests for translation encodings, native execution, and Python-binding input validation, plus Android APK build checks.
- A Python basic-block planner that splits supported instruction sequences around relative unconditional branches, builds explicit successor edges, and rejects conditional, absolute, linked, or out-of-range control flow.

The ELF-to-CPU test uses a synthetic ELF with simple instructions. The native loader now maps PT_LOAD segments and the bounded guest runner can fetch and execute the supported instruction subset from the ELF entry point. This is still a synthetic ELF test, not compatibility with a real PS3 executable. These tests validate a narrow prototype path only; they do not demonstrate compatibility with a real PS3 executable.

## Important limitations

- The new native API is a **small arithmetic proof of concept**, not a complete connection from every Python translator output or the integrated engine. The integrated engine continues to report `native_execution: false`.
- The supported PowerPC instruction set is tiny and does not represent a complete PPU implementation. The current native execution API accepts only 1–256 32-bit instruction words and only the explicitly supported subset. `lwz` reads are permitted only through the supplied callback; unmapped reads fail explicitly.
- Generated ARM64 execution requires an AArch64 process. On x86-64 hosts, tests verify translation, rejection behavior, and the unsupported-architecture response.
- Full PowerPC condition-register behavior, broad branch handling, SPU emulation, PS3 memory/OS services, and RSX graphics are not implemented. Stack-backed GPR storage is implemented only for the supported straight-line instruction subset.
- No PS3 title has booted in this project. The graphics demo activities are standalone prototypes, not PS3 rendering.

## Build and test

With CMake, a C compiler, and Python 3.12 installed:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python -m unittest discover -s tests -v
```

To build the optional native Python extension, configure with `-DPS3_TRANSLATOR_BUILD_PYTHON_BINDING=ON`. On an AArch64 machine, a supported example can be run through the API:

```python
from prototype.native_execution import execute_ppc_program_native

# addi r3,r0,7; addi r4,r3,5; add r5,r3,r4; subf r6,r3,r5
words = [0x38600007, 0x38830005, 0x7CA32214, 0x7CC32850]
print(execute_ppc_program_native(words, 6))  # expected: 12
```

Do not pass arbitrary machine-code bytes to `execute_u64`: it is a low-level trusted-code API, not a sandbox. Prefer the bounded PowerPC instruction API for the prototype tests.

The CI workflow runs host tests, attempts the translated-code execution test on a native ARM64 runner, and builds the Android debug APK. A green build verifies only the tests that exist; it does **not** prove that PS3 games are compatible.

## Current priorities

1. Confirm the new ELF-entry execution test, Android APK build, and native translation tests pass in CI.
2. Expand PPU instruction semantics with tests and differential checks against a trusted reference.
3. Add proper guest exceptions, stack/process setup, relocations, and imports before attempting real executable startup.
4. Integrate the guest runner with a real memory map and a minimal runtime/system-call layer.
5. Build out the much larger PPU/SPU, memory, OS, and RSX graphics requirements needed by a real emulator.

See [the 48-hour plan](docs/NEXT_48_HOURS.md), [register spill design](docs/REGISTER_SPILL_DESIGN.md), and [native execution status](docs/NATIVE_EXECUTION_STATUS.md). The [tiny PowerPC64 ELF fixture instructions](docs/TINY_ELF_TEST.md) and [project progress log](docs/PROJECT_PROGRESS.md) track the controlled executable milestone.

For the practical route toward real Android gameplay, see [Fastest path to PS3 gameplay](docs/FASTEST_PATH_TO_PS3_GAMEPLAY.md).

## Continuous integration

The `.github/workflows/prototype-validation.yml` workflow runs host-side C/Python tests, an ARM64 native execution job, and an Android debug APK build. When the Android build job succeeds, its APK is uploaded as the `ps3-translator-prototype-debug` workflow artifact.

A successful build proves that the prototype compiles and its tests pass; it does not prove that any PS3 game is compatible.

## Next practical milestone

The guest runner can now fetch big-endian instructions only from mapped executable regions, start at a supplied entry point, execute a bounded subset, and stop safely on unsupported instructions, invalid memory, or the step limit. The synthetic integration test loads an ELF, starts at its entry point, and verifies a register result. Next, expand instruction coverage and process/runtime setup. Keep unsupported instructions rejected explicitly. Full PS3 game support remains a much larger project requiring CPU, SPU, memory, operating-system, and graphics emulation.
