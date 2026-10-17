# PS3 Game-Running Goal: Project Reality and Build Plan

**Last updated:** 2026-10-17  
**Repository:** https://github.com/bhuvanking5/ps3-android-translator

## Important truth

This repository is currently an experimental PowerPC-to-ARM64 research prototype. **It does not currently run a commercial PS3 game.** A Markdown file can document the design and steps, but cannot itself turn the prototype into a working emulator or contain the missing emulator implementation.

The goal is to reach a verified milestone where a legally obtained PS3 game boots and runs on an Android device. Do not label the project game-capable until that test is demonstrated and reproducible.

## Fastest route to trying a real PS3 game

Before building a complete emulator from scratch, evaluate an existing Android PS3 emulator project:

- [ARMSX3 official repository](https://github.com/ARMSX2/ARMSX3)
- [ARMSX3 official releases](https://github.com/ARMSX2/ARMSX3/releases)

The official release page is the source of truth for APK variants, supported Android versions, requirements, and known issues. An older-device build may be worth testing on Android 11, but compatibility is not guaranteed on a Xiaomi Mi A3 / Snapdragon 665. Expect many games to be too demanding or unsupported.

Use only firmware and game files you are legally entitled to use, follow the emulator's official setup instructions, and download APKs only from the project's official release page. Do not assume that the emulator includes PS3 firmware.

## Why our current prototype cannot yet run a real game

A real PS3 title needs far more than a few PowerPC instructions. Major missing pieces include:

- PS3 executable and SELF decryption/validation support where applicable
- Accurate PPU instruction coverage, exceptions, registers, and memory behavior
- SPU processing units and their runtime
- PS3 operating-system APIs, threads, synchronization, files, and system services
- Executable loading, relocations, imports, and process initialization
- RSX graphics command processing and a graphics backend
- Audio, controllers, timing, and compatibility fixes
- End-to-end tests using a legally obtained homebrew test program, followed by game-specific validation

Implementing these components is a large engineering project; simply increasing the instruction count or changing the APK package will not be enough.

## Build strategy

### Phase 1 — Establish a trustworthy baseline
- Keep the prototype's host tests and Android builds green.
- Fix failing CI tests before adding more features.
- Preserve small reproducible tests for the ELF loader and guest runner.

### Phase 2 — Pick a core strategy
- **Preferred for real-game experimentation:** evaluate and use a maintained, license-compliant PS3 emulator core/project rather than rewriting the entire system.
- **For this repository:** only pursue core integration after checking its license, Android build instructions, native dependencies, device requirements, and whether integration is realistically maintainable.
- Do not copy large amounts of another emulator's code without understanding and complying with its license.

### Phase 3 — Demonstrate a bootable test program
- Add a minimal, legally distributable PS3 homebrew test ELF.
- Implement the required loader, runtime, and OS interfaces for that program.
- Verify boot, logging, memory behavior, and clean exit in automated tests.

### Phase 4 — Demonstrate a real game
- Select one legally obtained game and record its exact version and required setup.
- Test boot, graphics, audio, controls, and stability on actual hardware.
- Record results honestly; FPS and compatibility must be measured, not guessed.

## Definition of done

The project can claim **“runs a PS3 game”** only after all of these are documented:

1. The exact Android device and OS.
2. The exact emulator build/commit.
3. The game and version used, with no copyrighted game files committed to this repository.
4. Reproducible setup steps.
5. Evidence that gameplay actually starts, not just that the APK builds or a test ELF executes.
6. Measured performance and known problems.

## Current status

- Prototype CPU subset and guest ELF loader/runner: experimental.
- Tiny synthetic ELF test: intended to validate a few supported instructions and bounded execution; **not a PS3 game**.
- Real commercial PS3 game booted by this repository: **not demonstrated**.
- Performance target for the Xiaomi Mi A3: **unknown and not guaranteed**.

## Useful links

- [Repository README](../README.md)
- [Project progress log](PROJECT_PROGRESS.md)
- [Fastest path to PS3 gameplay](FASTEST_PATH_TO_PS3_GAMEPLAY.md)
- [GitHub Actions / CI checks](https://github.com/bhuvanking5/ps3-android-translator/actions)
