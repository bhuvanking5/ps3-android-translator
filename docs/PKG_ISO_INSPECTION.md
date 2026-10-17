# Read-only PS3 container inspection

## Signature checks

Run:

```sh
python tools/inspect_ps3_container.py /path/to/file.pkg
python tools/inspect_ps3_container.py /path/to/disc.iso
```

This performs bounded signature reads for PS3 PKG magic, ELF magic, ISO 9660
volume-descriptor identifiers, and a few UDF recognition/anchor signatures.
A matching signature does not prove that a file is complete, valid, a PS3 title,
or executable.

The Android app also performs a bounded ISO 9660/UDF signature scan when the
quick ELF/PKG/SELF checks do not match. It reports detection only; it does not
mount a disc image or launch it.

## ISO 9660 directory metadata

Run:

```sh
python tools/inspect_iso9660.py /path/to/disc.iso
```

The read-only parser validates the ISO 9660 primary volume descriptor, 2048-byte
block geometry, both-endian metadata fields, root directory record, directory
record boundaries, and file/directory extents. It lists paths, sizes, extents,
and directory flags without extracting file contents. Hard limits cap directory
entries and nesting depth.

This parser currently supports ISO 9660 metadata only. It does **not** traverse
UDF-only filesystems, parse PS3 PKG entry tables, install packages, decrypt
SELF/retail content, load an executable, emulate PS3 system services, or start
a game. It is not proof of PS3 disc compatibility.

## Remaining implementation milestones

1. Add a validated UDF directory traversal path and broaden malformed-image tests.
2. Add non-decrypting PKG metadata parsing and entry-table bounds checks.
3. Locate supported, unencrypted homebrew executables inside user-authorized
   containers and hand them to a real process loader.
4. Implement process entry/stack state, imports, and the required PS3 OS
   services before attempting game execution.
5. Add broader PPU/SPU semantics and RSX graphics; then test on actual devices.

Encrypted retail content and copy-protection bypass are out of scope. No real
PS3 game boot has been confirmed by this repository.
