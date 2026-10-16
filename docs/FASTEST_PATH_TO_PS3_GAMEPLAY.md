# Fastest honest path toward PS3 gameplay on Android

## Goal

The long-term goal is to play PS3 games on Android. The current repository is a small PowerPC-to-AArch64 research prototype; it is not a game-capable emulator.

## Key finding: reuse a complete emulator core

[ARMSX3](https://github.com/ARMSX2/ARMSX3) is an Android port of RPCS3 and is the relevant upstream project to study for real PS3 emulation. It already contains the much larger CPU, SPU, graphics, and system-emulation codebase. This repository should not attempt to recreate that scope in a short sprint.

Do not copy isolated files from ARMSX3/RPCS3 and assume they will work here. Core integration requires its build system, submodules, dependencies, license compliance, Android JNI integration, and substantial native compilation. The ARMSX3 build documentation reports a large native core build and requires Android SDK/NDK, CMake, and JDK tooling.

## Hardware constraint

A performance goal such as 30–45 FPS depends heavily on the particular game, CPU/GPU, Vulkan driver, thermals, and emulator maturity. It cannot be guaranteed by translating a few instructions or building an APK.

Before promising performance, record the device SoC, RAM, Android version, GPU/Vulkan support, and test a specific game using a compatible emulator build. Devices substantially below the upstream project's stated hardware targets should be expected to struggle, especially in demanding games.

## Practical short-sprint plan

1. Keep this prototype and its tests as a research component; do not delete the existing work.
2. Build and test the current Android app through CI, and fix compile or manifest failures.
3. Review ARMSX3's license and integration/build requirements before deciding whether to contribute upstream, build it separately, or make this repository a companion launcher/tool.
4. Make no claim of PS3 game support until a legally obtained game and firmware setup has actually reached gameplay on the target device.
5. Record real frame rate and stability for each tested game. Do not use a fixed smoke test as evidence of game compatibility.

## Definition of success for the next 1–2 hours

A reproducible successful build, clear device compatibility findings, and a tested next step are achievable milestones. A new PS3 emulator capable of 30–45 FPS across games is not realistically achievable from this prototype in that time.
