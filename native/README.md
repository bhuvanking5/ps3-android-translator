# Native execution bridge (prototype)

This directory contains an experimental bridge between Python and the native runtime.

## What this adds

- `native/python_binding.c` exposes `execute_u64(bytes)` as an optional CPython extension.
- The extension passes a byte buffer to the native runtime and returns the function's `x0` value.
- GitHub Actions builds this extension on its Linux host, so the C/Python boundary is compile-tested.

## Build on a compatible host

```sh
cmake -S . -B build -DPS3_TRANSLATOR_BUILD_PYTHON_BINDING=ON
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
```

The resulting module is a host CPython extension, not an Android APK or Android JNI library. Android needs an Android NDK build and a JNI boundary.

## Critical safety and correctness limits

- This API executes supplied bytes as native machine code. Only pass code produced and validated by trusted local code. It is not a sandbox and must never be used with untrusted game files or downloaded bytecode.
- It accepts only a tiny no-argument function returning a 64-bit value in `x0`; it is not a general PPC execution engine.
- The runtime's actual machine-code execution test runs only on AArch64. On x86-64 CI, argument checks run but execution returns an unsupported-architecture error.
- Executable memory may be disallowed by platform policy. Android's memory protections and app policies must be tested on a real device.
- The current translator is not connected to this API yet. Do not call this feature a working PS3 emulator.
