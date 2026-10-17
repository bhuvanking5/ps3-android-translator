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

The fixture generation, ELF PT_LOAD loading, bounded entry-point runner, and synthetic final-register integration test are already present. CI has also passed host C/Python tests, generated-code execution on an ARM64 runner, and an Android debug APK build for the previous validated revision.

Next engineering milestones:

1. Add an explicit, tested guest stop/exit contract so a guest program can finish cleanly instead of relying on an artificial branch loop.
2. Define minimal PowerPC process-entry state (including stack and required register conventions) and test it independently of game code.
3. Expand instruction semantics in small batches with reference-vs-translated differential tests; reject unsupported instructions explicitly.
4. Only after those foundations, try a real, legally obtained homebrew ELF. Do not treat commercial game boot as an expected result yet.

## Android ELF launch-pipeline test (2026-10-17)

The Android UI now has a **Run built-in ELF execution test** button. It synthesizes a tiny, unencrypted ELF64 big-endian PowerPC64 executable in the native bridge and routes it through the same ELF loader and bounded guest interpreter used by the selected-ELF path. The test sets r3=7, r4=5, computes r5=12, and then stops at the runner's instruction budget. The status line includes the resulting registers.

This is a useful device-side smoke test for the JNI → ELF loader → guest memory → PowerPC interpreter path. It is **not a PS3 game**, and reaching the step limit is expected for this fixture's intentional self-branch. The selected-file path is limited to raw ELF files no larger than 32 MiB, and the current guest RAM window is 64 MiB. PKG extraction/installation, SELF decryption, dynamic linking, process/stack setup, system services, SPU, and RSX remain missing. No real PS3 game has booted.

## Additional immediate logical coverage (2026-10-17)

The bounded native PowerPC interpreter and experimental AArch64 translator now include ORIS and XORIS (opcodes 25 and 27) alongside ORI/XORI. These instructions shift the 16-bit immediate left by 16 before applying OR/XOR. Regression tests cover the C reference interpreter, Python reference decoder/execution, translation acceptance, and the native ARM64 differential test path.

This remains a narrow instruction-subset improvement. It does not add full PPU semantics, dynamic block caching, PS3 system services, SPU/RSX support, or real-game compatibility.


## Read-only PKG/ISO signature inspector (2026-10-17)

Added `tools/inspect_ps3_container.py` and unit tests for bounded signature
recognition of PKG magic, ELF magic, ISO 9660 volume descriptors, and basic
UDF recognition/anchor signatures. This makes file identification testable
without pretending a recognized header means a game can run. The inspector is
a host-side diagnostic and is not yet connected to the Android app.

The tool does not extract/install PKG files, traverse ISO/UDF directories,
decrypt protected content, locate an executable, or launch a game. The project
still has **zero confirmed real-game boots**. Next container milestone is
bounds-checked directory and package metadata parsing, followed by loading a
supported unencrypted homebrew executable through a proper process runtime.
