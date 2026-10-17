# Read-only PS3 container signature inspection

Run:

```sh
python tools/inspect_ps3_container.py /path/to/file.pkg
python tools/inspect_ps3_container.py /path/to/disc.iso
```

The inspector performs bounded signature reads for PS3 PKG magic, ELF magic,
ISO 9660 volume-descriptor identifiers, and a few UDF recognition/anchor
signatures. It is a diagnostic, not a loader. A matching signature does not
prove a file is complete, valid, a PS3 title, or executable.

It deliberately does **not** extract or install packages, decrypt SELF/retail
content, mount a disc image, enumerate a game filesystem, locate an executable,
or start a game. The Android app does not yet use this Python tool; its
existing selected-file path remains limited to raw ELF input.

## Remaining implementation milestones

1. Add validated ISO/UDF directory traversal with strict bounds checks.
2. Add non-decrypting PKG metadata parsing and entry-table bounds checks.
3. Locate supported, unencrypted homebrew executables inside user-authorized
   containers and hand them to a real process loader.
4. Implement process entry/stack state, imports, and the required PS3 OS
   services before attempting game execution.
5. Add broader PPU/SPU semantics and RSX graphics; then test on actual devices.

Encrypted retail content and copy-protection bypass are out of scope. No real
PS3 game boot has been confirmed by this repository.
