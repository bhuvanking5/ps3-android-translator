# Native ARM64 execution status

## Implemented

- The C native runtime validates arguments and uses write-then-execute memory permissions on AArch64.
- A bounded C PowerPC-to-ARM64 translator emits a restricted instruction subset: ADDI, ADD, SUBF, MULLD, OR, AND, XOR, and local relative unconditional `b` branches. Branches are relocated after emitted instruction offsets are known; conditional, absolute, linked, and out-of-sequence branches are rejected.
- The reference interpreter supports signed-word `cmpwi` and a restricted local relative `bc` subset (`BO=4` or `BO=12`) using the selected condition-register bit. It also follows local relative unconditional `b` and has a hard step limit to stop infinite loops.
- The ARM64 translator still supports only local relative unconditional `b`; conditional branches and condition-register semantics are not yet translated. Linked branches, absolute branches, and out-of-sequence branch targets remain unsupported.
- The translator stores all 32 PowerPC GPRs in a 256-byte stack frame and uses x16/x17 as temporary registers; generated code restores SP before returning.
- The Android JNI smoke-test path runs a fixed, built-in PowerPC arithmetic program through both the reference interpreter and the C translator, executes the generated ARM64 words, and compares the result.
- The optional CPython extension now exposes `translate_and_execute_ppc(instruction_words, result_gpr)`. It accepts a sequence of 32-bit PowerPC words, enforces the instruction/register limits, translates only supported instructions, executes the generated ARM64 on AArch64, and compares the result with the reference interpreter.
- `prototype/native_execution.py` provides a Python wrapper around that API.
- Tests cover emitted words, unsupported instructions, invalid arguments, and the expected unsupported-architecture response on x86-64.
- CI includes a native ARM64 job intended to execute the translated program and compare its result with the PowerPC reference interpreter.

## Not yet verified

- The new reference-interpreter `cmpwi`/restricted-`bc` tests must pass CI before this interpreter extension is marked CI-verified.
- The native translator covers only a tiny arithmetic/logical subset plus local unconditional branches. The reference interpreter has a somewhat broader conditional-branch subset, so conditional-branch programs cannot yet be translated to ARM64. This is not a general PowerPC translator, a full integration of every Python translator output, or a real PS3 executable.
- The Android JNI build must pass, and the smoke test still needs to be exercised on a physical ARM64 Android device.
- No PS3 title has been run.

## Acceptance criteria for this milestone

1. Pass C translation and encoding tests.
2. Pass Python-binding input-validation tests.
3. Pass native execution comparison on an AArch64 runner, with generated result matching the reference interpreter.
4. Build the Android APK with the new JNI path.
5. Run the app on an ARM64 Android device and confirm its native status message reports the translated smoke test passed.

Until the CI run and Android build have passed, describe this as implemented but still being verified. Even after they pass, this milestone is only a tiny CPU-translation proof of concept, not PS3 game compatibility.
