# PS3 Android Translator (research prototype)

Experimental research project exploring translation of a **small subset of PowerPC instructions to ARM64** as one possible building block for Android.

> **Status:** early prototype, not a PS3 emulator and not capable of running PS3 games.

## What exists today

- A small PowerPC instruction decoder and reference interpreter.
- Experimental ARM64 instruction encoding for a limited instruction subset.
- A basic block translator with limited local relative branch support.
- Python unit tests run by GitHub Actions.
- An integrated validation path that compares the reference interpreter's state with translation checks.

## Important limitations

- Generated ARM64 bytes are **not yet executed** by the integrated engine; its report explicitly marks `native_execution: false`.
- The supported PowerPC instruction set is tiny and does not represent a complete PPU implementation.
- Register `r31` has no real spill/load implementation. The simple pipeline now rejects translations that use it rather than pretending ARM64 `x31` is an ordinary general-purpose register.
- Branch support is limited; linking, absolute branches, and many control-flow cases are unsupported.
- There is no complete PS3 system emulation, SPU emulation, memory model, operating-system layer, or graphics implementation.

## Run tests

On a computer with Python 3.12:

```sh
python -m unittest discover -s tests -v
```

GitHub Actions runs the same test command on pushes and pull requests. A green test run verifies the tests that exist; it does **not** prove native ARM64 execution or PS3 game compatibility.

## Current priorities

1. Keep the supported subset correct and fail clearly on unsupported instructions.
2. Add regression tests for instruction encodings, branches, register mapping, and edge cases.
3. Design and implement proper register spill/load support before translating instructions that require it.
4. Establish a reproducible ARM64 native-execution test harness before claiming native execution.
5. Expand instruction coverage only alongside reference-versus-translator tests.

See [the 48-hour plan](docs/NEXT_48_HOURS.md), [register spill design](docs/REGISTER_SPILL_DESIGN.md), and [native execution status](docs/NATIVE_EXECUTION_STATUS.md).
