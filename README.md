# PS3 Android Translator (research prototype)

Experimental research project exploring translation of a **small subset of PowerPC instructions to ARM64** as one possible building block for Android.

> **Status:** early prototype, not a PS3 emulator and not capable of running PS3 games.

## What exists today

- A small PowerPC instruction decoder and reference interpreter, including `or` register logic and bounded execution directly from guest-memory buffers for a tiny instruction subset.
- A defensive PS3-style ELF64/PowerPC64 metadata inspector and bounded PT_LOAD segment mapper; this is not a complete SELF/ELF game loader.
- Experimental ARM64 instruction encoding for a limited instruction subset, including register-form `ORR`, 64-bit `MUL`, and PowerPC `mulld` decode/reference-execution/translation coverage. The record-setting form remains explicitly unsupported.
- A basic block translator with limited local relative branch support, plus explicit rejection tests for unsupported absolute/linked branches, `r31`, record-form `add`, and misaligned program addresses.
- A C native runtime API and CMake build that produces a shared library.
- Python unit tests and a native-runtime smoke test run by GitHub Actions.

The ELF-to-CPU test uses a synthetic ELF with two simple `addi` instructions. It validates this narrow prototype path only; it does not demonstrate compatibility with a real PS3 executable.

## Important limitations

- The generated ARM64 bytes are **not yet connected to the translator or Android app**; the integrated engine still reports `native_execution: false`.
- The native runtime smoke test validates API errors on the CI host. The actual machine-code execution test runs only on AArch64 and is skipped on the current x86-64 hosted runner.
- The supported PowerPC instruction set is tiny and does not represent a complete PPU implementation.
- Register `r31` has no real spill/load implementation. The simple pipeline rejects translations that use it rather than pretending ARM64 `x31` is an ordinary general-purpose register.
- Branch support is limited; linking, absolute branches, conditional branches in generated ARM64, and many control-flow cases are unsupported. The safety tests ensure known unsupported cases fail clearly rather than being silently translated.
- There is no complete PS3 system emulation, SPU emulation, memory model, operating-system layer, or graphics implementation.

## Build and test

With CMake, a C compiler, and Python 3.12 installed:

```sh
cmake -S . -B build
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python -m unittest discover -s tests -v
```

The CMake build produces the `ps3_arm64_runtime` shared library and a smoke-test executable. A green test run verifies only the tests that exist; it does **not** prove that ARM64 code executed on the CI host or that PS3 games are compatible.

## Current priorities

1. Connect the translator's machine-code output to the native runtime through a well-defined API.
2. Add an Android NDK build and JNI boundary, then test on an actual ARM64 Android device.
3. Establish a reproducible ARM64 native-execution test harness.
4. Implement correct register spill/load support and broader instruction coverage with reference-versus-translator tests.
5. Work toward the much larger PPU/SPU, memory, OS, and graphics requirements of a PS3 emulator.

See [the 48-hour plan](docs/NEXT_48_HOURS.md), [register spill design](docs/REGISTER_SPILL_DESIGN.md), and [native execution status](docs/NATIVE_EXECUTION_STATUS.md).

For the practical route toward real Android gameplay, see [Fastest path to PS3 gameplay](docs/FASTEST_PATH_TO_PS3_GAMEPLAY.md).

## Continuous integration

The `.github/workflows/prototype-validation.yml` workflow runs the host-side C/Python test suites and builds the Android debug APK. When the Android build job succeeds, its APK is uploaded as the `ps3-translator-prototype-debug` workflow artifact.

A successful build proves that the prototype compiles and that its existing tests pass; it does not prove that generated ARM64 code is executed by the translator, nor that any PS3 game is compatible.

## Next practical milestone

Connect the actual translator output to a narrowly scoped native execution test, compare the result with the reference interpreter, and run that test on an ARM64 Android device. Keep unsupported instructions rejected explicitly. Full PS3 game support remains a much larger project requiring CPU, SPU, memory, operating-system, and graphics emulation.
