# Project progress log

Last updated: 2026-10-17 (UTC)

## Current honest status

This repository is an experimental PowerPC-to-ARM64 research prototype, **not a working PS3 emulator**. No real PS3 game has booted. Do not use percentage estimates as evidence of compatibility or performance.

## Latest completed work

- The bounded PowerPC interpreter rejects record-form arithmetic/logical instructions when condition-register updates are not implemented.
- The guest-memory ELF entry-point runner now applies the same record-form rejection rule; a regression test covers this behavior.
- CI previously passed host C/Python tests, ARM64 generated-code execution, and Android debug APK build for commit d0e86900cad0d6ebe104001ce3797de0c61f9920.
- Added tools/make_tiny_ps3_elf.py and tests/test_tiny_ps3_elf.py to generate and validate an unencrypted ELF64 big-endian PowerPC64 fixture with a small supported instruction sequence. Added docs/TINY_ELF_TEST.md and linked this progress log from README.

## Tiny executable fixture

Fixture generator and format tests are now committed. The guest-runner regression test also verifies that unsupported record-form instructions stop explicitly. CI status for these newest commits is pending at the time of this update.

Generate the test input with Python 3:

~~~sh
python tools/make_tiny_ps3_elf.py
~~~

This creates tiny_ps3_test.elf, an ELF64 big-endian PowerPC64 ET_EXEC with one RX PT_LOAD segment. Its guest instructions set r3=7, r4=5, calculate r5=12, then branch in place.

**Important:** generating this fixture does not make the Android app execute the selected file. It is a controlled test input for developing and testing the native loader/runner. The prototype still lacks the full PS3 SELF loader, relocations/imports, process setup, OS services, broad PPU/SPU support, and RSX graphics.

## Next milestones

1. Verify CI for the fixture and guest-runner regression tests.
2. Connect a test ELF fixture to a native integration test that checks the final guest register state.
3. Add minimal guest process/runtime setup and explicit syscall/exit handling for a clean stop condition.
4. Only then try a real, legally obtained homebrew ELF; do not treat commercial game boot as an expected result yet.