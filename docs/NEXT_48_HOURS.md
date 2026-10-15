# Next Moves: 48-Hour Engineering Plan

Current rough project progress: **7%** toward the long-term goal of running real PS3 games on Android.

This percentage is an engineering estimate, not a benchmark. Passing Python tests does not mean generated ARM64 code has executed, and the project is not yet a PS3 emulator.

## Constraints

- Development is being done from an Android phone and tablet using the browser.
- Do not require Termux or a local PC.
- Make small, reviewable commits and run GitHub Actions after each change.
- Never claim native execution unless an ARM64 execution test actually ran and its output was checked.

## Day 1 — Correctness and regression tests

1. Inspect the PowerPC-to-ARM64 register mapping and fix the r31 limitation safely. ARM64 x31 is SP/ZR depending on instruction context; it is not a normal general-purpose register. Do not pretend a label such as `spill_r31` is a working spill implementation.
2. Add focused tests for register mapping boundaries (r0, r30, r31, invalid indices, and booleans).
3. Add tests for supported instruction encodings, immediate boundaries, branch alignment/range, and branches to valid/invalid block targets.
4. Run the full GitHub Actions test suite. Fix regressions before adding features.

## Day 2 — Native execution proof, if a suitable runner is available

1. Check whether an available CI runner supports ARM64 and can safely execute generated AArch64 code.
2. If so, create a minimal native test harness for a tiny, known instruction sequence and compare its result with the Python reference interpreter.
3. If no suitable runner is available, do not fake the test. Improve the translator's differential tests and document native execution as blocked on an ARM64-capable runtime.
4. Record the exact test command, environment, and results.

## Definition of a real native-execution milestone

A milestone counts only when:
- generated ARM64 bytes are executed by an ARM64 CPU/runtime (not just printed or hex-encoded);
- the test checks the result against the reference interpreter;
- the test runs reproducibly in CI or another documented environment.

## Current known limitations

- The integrated engine currently validates translated bytes but reports `native_execution: false`.
- The Python reference interpreter is not a native ARM64 execution engine.
- Branch translation only supports a limited subset of local, relative, non-linking branches.
- A PS3 emulator additionally needs extensive CPU, memory, system, and graphics support; this prototype is an early foundation, not a game-ready emulator.

## Progress reporting rule

Keep the overall percentage at 7% until a substantial, verified capability is added. Do not increase it merely because files or tests were added. Report completed tests and limitations alongside any future percentage change.
