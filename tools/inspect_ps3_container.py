#!/usr/bin/env python3
"""Read-only PS3 container signature inspector.

This utility recognizes a few container/file signatures only. It does not
extract, decrypt, install, mount, or execute content.
"""
from __future__ import annotations

import argparse
from pathlib import Path
from typing import BinaryIO

SECTOR_SIZE = 2048
ISO_DESCRIPTOR_START = 16
ISO_DESCRIPTOR_COUNT = 32
UDF_IDENTIFIERS = {b"BEA01", b"NSR02", b"NSR03", b"TEA01"}


def inspect_stream(stream: BinaryIO, size: int) -> tuple[str, str]:
    """Return (kind, explanation) using bounded signature reads."""
    magic = stream.read(4)
    if magic == b"\x7fPKG":
        return (
            "PS3_PKG_SIGNATURE",
            "PKG magic matches; package structure/content is NOT validated and nothing was extracted.",
        )
    if magic == b"\x7fELF":
        return (
            "ELF_SIGNATURE",
            "ELF magic found; architecture, segments, encryption, and executability are NOT validated.",
        )
    if size < 4:
        return "TOO_SMALL", "File is too small to contain a recognized header."

    # ISO 9660 primary/supplementary volume descriptors begin at logical
    # sector 16. A descriptor has its identifier at byte offset +1.
    for sector in range(ISO_DESCRIPTOR_START, ISO_DESCRIPTOR_START + ISO_DESCRIPTOR_COUNT):
        offset = sector * SECTOR_SIZE
        if size < offset + 6:
            break
        stream.seek(offset + 1)
        ident = stream.read(5)
        if ident == b"CD001":
            return (
                "ISO9660_SIGNATURE",
                "ISO 9660 volume-descriptor signature found; this alone does not prove it is a PS3 disc image.",
            )
        if ident in UDF_IDENTIFIERS:
            return (
                "UDF_SIGNATURE",
                f"UDF recognition identifier {ident.decode('ascii')} found; disc contents and PS3 layout are NOT validated.",
            )

    # UDF anchor volume descriptor commonly appears at logical sector 256.
    anchor_offset = 256 * SECTOR_SIZE
    if size >= anchor_offset + 2:
        stream.seek(anchor_offset)
        tag_id = int.from_bytes(stream.read(2), "little")
        if tag_id == 2:  # ECMA-167 Anchor Volume Descriptor Pointer
            return (
                "UDF_ANCHOR_CANDIDATE",
                "UDF anchor descriptor tag found at sector 256; descriptor integrity and filesystem traversal are not validated.",
            )

    return (
        "UNKNOWN",
        "No supported PKG, ELF, ISO 9660, or basic UDF signature was found at checked offsets.",
    )


def inspect_path(path: Path) -> tuple[str, str]:
    with path.open("rb") as stream:
        size = path.stat().st_size
        return inspect_stream(stream, size)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("file", type=Path, help="path to a file you are authorized to inspect")
    args = parser.parse_args()
    try:
        kind, message = inspect_path(args.file)
    except OSError as exc:
        parser.error(str(exc))
    print(f"Type: {kind}")
    print(message)
    print("Read-only signature check only; no extraction, decryption, installation, or launch occurred.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
