# PS3 Android Translator (research prototype)

Experimental research project exploring translation of a **small subset of PowerPC instructions to ARM64** as one possible building block for Android.

> **Status:** early prototype, not a PS3 emulator and not capable of running PS3 games.

## What exists today

- A small PowerPC instruction decoder and reference interpreter, including logical operations and bounded execution for a tiny instruction subset.
- A defensive PS3-style ELF64/PowerPC64 metadata inspector and bounded PT_LOAD segment mapper; this is not a complete SELF/ELF game loader.
- Experimental Python instruction encoding and translation tools for a limited instruction subset.
- A new bounded C PowerPC-to-ARM64 translator for ADDI, ADD, SUBF, MULLD, OR, AND, and XOR. It rejects unsupported instructions and restricts the register mapping to caller-saved ARM64 registers.
- An Android JNI smoke-test path that translates a **fixed built-in PowerPC arithmetic program**, executes the generated ARM64 words, and compares the result against the reference interpreter. It never accepts executable bytes from a selected game file.
- CMake tests for translation encodings and for executing the translated program on AArch64, plus Python unit tests and Android APK build checks.
- A basic block translator with limited local relative branch support and explicit rejection tests for unsupported control-flow cases.

The ELF-to-CPU test uses a synthetic ELF with simple instructions. It validates this narrow prototype path only; it does not demonstrate compatibility with a real PS3 executable.

## Important limitations

- The new C translator and Android bridge cover only a tiny fixed arithmetic test. They are **not yet a complete connection to the Python translator or integrated engine**, which continues to report `native_execution: false`.
- The generated-code execution test only proves the small supported subset when it passes on an AArch64 runtime. An x86-64 host can validate translation and error handling but cannot execute AArch64 instructions.
- The supported PowerPC instruction set is tiny and does not represent a complete PPU implementation.
- Register spill/load support, full PowerPC condition-register behavior, broad branch handling, SPU emulation, PS3 memory/OS services, and RSX graphics are not implemented.
- No PS3 title has booted in this project. The graphics demo activities are standalone prototypes, not PS3 rendering.

## Build and test

With CMake, a C compiler, and Python 3.12 installed:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python -m unittest discover -s tests -v
```

The CI workflow runs host tests, attempts the translated-code execution test on a native ARM64 runner, and builds the Android debug APK. A green build verifies only the tests that exist; it does **not** prove that PS3 games are compatible.

## Current priorities

1. Confirm generated-code execution on the ARM64 CI runner and on an actual ARM64 Android device.
2. Connect the Python translator's supported output to the native execution API with explicit validation and reference-result comparisons.
3. Add correct register spill/load support, condition-register behavior, and broader instruction coverage with differential tests.
4. Build out the much larger PPU/SPU, memory, OS, and graphics requirements needed by a real emulator.

See [the 48-hour plan](docs/NEXT_48_HOURS.md), [register spill design](docs/REGISTER_SPILL_DESIGN.md), and [native execution status](docs/NATIVE_EXECUTION_STATUS.md).

For the practical route toward real Android gameplay, see [Fastest path to PS3 gameplay](docs/FASTEST_PATH_TO_PS3_GAMEPLAY.md).

## Continuous integration

The `.github/workflows/prototype-validation.yml` workflow runs host-side C/Python tests, an ARM64 native execution job, and an Android debug APK build. When the Android build job succeeds, its APK is uploaded as the `ps3-translator-prototype-debug` workflow artifact.

A successful build proves that the prototype compiles and its tests pass; it does not prove that any PS3 game is compatible.

## Next practical milestone

Verify the bounded translated-code test on the ARM64 runner and a physical Android device. Then connect the Python translation pipeline to the same validated execution API. Keep unsupported instructions rejected explicitly. Full PS3 game support remains a much larger project requiring CPU, SPU, memory, operating-system, and graphics emulation.
