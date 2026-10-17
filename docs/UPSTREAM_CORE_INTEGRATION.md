# Upstream PS3 Core Integration

## Decision

The fastest credible route to a game-capable Android PS3 emulator is to evaluate and build on the existing ARMSX3/RPCS3 ARM64 core, rather than trying to grow the small instruction-subset interpreter into a complete PS3 emulator.

This repository now pins ARMSX3 as a Git submodule at `upstream/ARMSX3`:

- Upstream: https://github.com/ARMSX2/ARMSX3
- Pinned upstream commit: `23e6daabc35fe866e20662028952605a75f96a82` (ARMSX3 1.2)
- License: GPL-2.0-only, matching RPCS3; review individual file headers before copying or modifying source.

The submodule is a source reference, not a completed merge. The current app still builds the experimental translator prototype. CI checkout does not recursively fetch submodules by default, so the existing small prototype test jobs do not download/build the large upstream tree.

## Upstream build constraints

The upstream build guide specifies Android NDK r27 or newer, CMake 3.30 or newer, JDK 17, and an Android SDK. It also requires recursive submodules plus two additional third-party checkouts. Its unstripped core library is reported at about 1.3 GB. The current prototype workflow uses CMake 3.22.1 and Android API 26 for its own small native bridge; those settings are not sufficient for an upstream-core build.

Reference: https://github.com/ARMSX2/ARMSX3

## Integration milestones

1. **Pinned source (done):** record a specific upstream commit as a submodule so updates are deliberate and reproducible.
2. **Upstream build validation (next):** use a dedicated CI job/runner with CMake 3.30+, NDK r27+, JDK 17, recursive submodules, and the two required third-party checkouts. Keep this separate from the prototype build.
3. **App boundary:** determine the smallest stable interface for loading the upstream native core from the Android app; do not try to link the existing toy interpreter as if it were a complete core.
4. **Device validation:** produce an APK, test installation on the target Android device, and record actual boot/log output.
5. **Gameplay evidence:** test legally owned firmware and game content. Only call the project game-capable after a real title reaches gameplay.

## Hardware reality

ARMSX3's published minimum target is substantially stronger than a Snapdragon 665 with 6 GB RAM. Pinning its source improves the architecture decision, but does not mean the target phone can run it at a useful frame rate. Treat device compatibility as an early go/no-go test rather than promising 30–45 FPS.

## Current status

- The prototype's existing CI tests and ARM64 cross-compile are separate from upstream-core validation.
- No commercial PS3 game has been demonstrated running from this repository.
- Do not report a 60–70% completion figure until the upstream core is integrated, the app boots it on-device, and there is reproducible gameplay evidence.
