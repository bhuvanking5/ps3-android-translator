# Native ARM64 execution status

## Implemented

- The C native runtime validates arguments and uses write-then-execute memory permissions on AArch64.
- A bounded C PowerPC-to-ARM64 translator now emits a restricted instruction subset: ADDI, ADD, SUBF, MULLD, OR, AND, and XOR.
- The translator rejects unsupported instructions and register numbers that would use ARM64 platform/callee-saved registers without a save/restore implementation.
- The Android JNI smoke-test path runs a fixed, built-in PowerPC arithmetic program through both the reference interpreter and the C translator, executes the generated ARM64 words, and compares the result. It never treats selected game-file bytes as executable code.
- Host C tests check the emitted instruction words and verify that non-AArch64 systems report execution as unsupported.
- CI now includes a native ARM64 job intended to execute the translated program and compare its result with the PowerPC reference interpreter.

## Not yet verified

- The current CI run must finish successfully before the new ARM64 execution path can be marked CI-verified.
- The test uses a fixed arithmetic program, not arbitrary output from the Python translator or a real PS3 executable.
- The integrated Python engine still reports `native_execution: false`; the Python pipeline has not yet been connected end-to-end to the native execution API.
- The Android JNI build must pass, and the smoke test still needs to be exercised on a physical ARM64 Android device.
- No PS3 title has been run.

## Acceptance criteria for the next milestone

1. Pass the C translation/encoding tests.
2. Pass the native execution comparison on an AArch64 runner, with generated result matching the reference interpreter.
3. Build the Android APK with the new JNI path.
4. Run the app on an ARM64 Android device and confirm its native status message reports the translated smoke test passed.
5. After that, connect the Python translator output to the native API through a narrow, explicitly validated interface.

Until the ARM64 execution job and Android build have passed, describe this as implemented but still being verified. Even after they pass, this milestone is only a tiny CPU-translation proof of concept, not PS3 game compatibility.
