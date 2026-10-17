#!/usr/bin/env python3
"""Read-only, bounds-checked ISO 9660 directory listing for PS3 disc research.

This lists metadata only. It does not extract, decrypt, or execute files.
"""
from __future__ import annotations

import argparse
import io
import struct
from dataclasses import dataclass
from pathlib import Path
from typing import BinaryIO

SECTOR_SIZE = 2048
DESCRIPTOR_START = 16
DESCRIPTOR_COUNT = 32
MAX_ENTRIES = 10000
MAX_DEPTH = 16


class IsoFormatError(ValueError):
    """The input is truncated, malformed, or exceeds parser safety limits."""


@dataclass(frozen=True)
class IsoEntry:
    path: str
    extent_lba: int
    size: int
    is_directory: bool


@dataclass(frozen=True)
class IsoImage:
    volume_blocks: int
    block_size: int
    entries: tuple[IsoEntry, ...]


def _read_at(stream: BinaryIO, offset: int, count: int, size: int) -> bytes:
    if offset < 0 or count < 0 or offset > size or count > size - offset:
        raise IsoFormatError("read extends beyond the input image")
    stream.seek(offset)
    data = stream.read(count)
    if len(data) != count:
        raise IsoFormatError("truncated image data")
    return data


def _both_endian16(data: bytes, offset: int) -> int:
    if offset + 4 > len(data):
        raise IsoFormatError("truncated both-endian 16-bit field")
    little = int.from_bytes(data[offset:offset + 2], "little")
    big = int.from_bytes(data[offset + 2:offset + 4], "big")
    if little != big:
        raise IsoFormatError("inconsistent both-endian 16-bit field")
    return little


def _both_endian32(data: bytes, offset: int) -> int:
    if offset + 8 > len(data):
        raise IsoFormatError("truncated both-endian 32-bit field")
    little = int.from_bytes(data[offset:offset + 4], "little")
    big = int.from_bytes(data[offset + 4:offset + 8], "big")
    if little != big:
        raise IsoFormatError("inconsistent both-endian 32-bit field")
    return little


def _directory_record(data: bytes) -> tuple[int, int, int, bytes, bool]:
    if len(data) < 34 or data[0] != len(data):
        raise IsoFormatError("invalid ISO directory record length")
    identifier_length = data[32]
    if identifier_length == 0 or 33 + identifier_length > len(data):
        raise IsoFormatError("invalid ISO directory identifier length")
    extent = _both_endian32(data, 2)
    size = _both_endian32(data, 10)
    identifier = data[33:33 + identifier_length]
    is_directory = bool(data[25] & 0x02)
    return extent, size, data[25], identifier, is_directory


def parse_iso9660(stream: BinaryIO, image_size: int,
                  max_entries: int = MAX_ENTRIES,
                  max_depth: int = MAX_DEPTH) -> IsoImage:
    """Validate an ISO 9660 PVD and enumerate directory metadata with hard bounds."""
    if image_size < (DESCRIPTOR_START + 1) * SECTOR_SIZE:
        raise IsoFormatError("image is too small for an ISO 9660 volume descriptor")
    if max_entries < 1 or max_depth < 0:
        raise ValueError("limits must be positive (depth may be zero)")

    pvd = None
    for sector in range(DESCRIPTOR_START, DESCRIPTOR_START + DESCRIPTOR_COUNT):
        descriptor = _read_at(stream, sector * SECTOR_SIZE, SECTOR_SIZE, image_size)
        if descriptor[1:6] != b"CD001" or descriptor[6] != 1:
            continue
        if descriptor[0] == 1:
            pvd = descriptor
            break
        if descriptor[0] == 255:
            break
    if pvd is None:
        raise IsoFormatError("no valid ISO 9660 primary volume descriptor found")

    volume_blocks = _both_endian32(pvd, 80)
    block_size = _both_endian16(pvd, 128)
    if volume_blocks == 0 or block_size != SECTOR_SIZE:
        raise IsoFormatError("unsupported or invalid ISO volume geometry")
    if volume_blocks > image_size // block_size:
        raise IsoFormatError("declared volume extends beyond the image")

    root_len = pvd[156]
    if root_len < 34 or 156 + root_len > len(pvd):
        raise IsoFormatError("invalid root directory record")
    root_extent, root_size, _, root_id, root_is_dir = _directory_record(
        pvd[156:156 + root_len])
    if root_id != b"\x00" or not root_is_dir:
        raise IsoFormatError("invalid ISO root directory record")

    def check_extent(extent: int, length: int) -> None:
        if extent >= volume_blocks or length > (volume_blocks - extent) * block_size:
            raise IsoFormatError("file or directory extent exceeds declared volume")
        offset = extent * block_size
        if offset > image_size or length > image_size - offset:
            raise IsoFormatError("file or directory extent exceeds image size")

    check_extent(root_extent, root_size)
    queue: list[tuple[str, int, int, int]] = [("", root_extent, root_size, 0)]
    seen: set[tuple[int, int]] = set()
    entries: list[IsoEntry] = []

    while queue:
        parent, extent, length, depth = queue.pop(0)
        key = (extent, length)
        if key in seen:
            continue
        seen.add(key)
        check_extent(extent, length)
        raw = _read_at(stream, extent * block_size, length, image_size)
        cursor = 0
        while cursor < len(raw):
            record_len = raw[cursor]
            if record_len == 0:
                cursor = ((cursor // block_size) + 1) * block_size
                continue
            within_block = cursor % block_size
            if record_len < 34 or record_len > block_size - within_block:
                raise IsoFormatError("directory record crosses a sector or is too short")
            if cursor + record_len > len(raw):
                raise IsoFormatError("directory record extends beyond directory data")
            record = raw[cursor:cursor + record_len]
            child_extent, child_size, _, identifier, is_dir = _directory_record(record)
            cursor += record_len
            if identifier in (b"\x00", b"\x01"):
                continue
            name = identifier.decode("ascii", errors="replace")
            if ";" in name:
                name = name.rsplit(";", 1)[0]
            name = name.rstrip(".")
            path = f"{parent}/{name}" if parent else name
            check_extent(child_extent, child_size)
            entries.append(IsoEntry(path, child_extent, child_size, is_dir))
            if len(entries) > max_entries:
                raise IsoFormatError("directory entry limit exceeded")
            if is_dir:
                if depth >= max_depth:
                    raise IsoFormatError("directory nesting limit exceeded")
                queue.append((path, child_extent, child_size, depth + 1))

    return IsoImage(volume_blocks, block_size, tuple(entries))


def inspect_path(path: Path) -> IsoImage:
    with path.open("rb") as stream:
        return parse_iso9660(stream, path.stat().st_size)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path, help="ISO image you are authorized to inspect")
    args = parser.parse_args()
    try:
        result = inspect_path(args.image)
    except (OSError, IsoFormatError) as exc:
        parser.error(str(exc))
    print(f"ISO 9660 metadata valid: {result.volume_blocks} blocks x {result.block_size} bytes")
    print(f"Entries listed: {len(result.entries)}")
    for entry in result.entries:
        kind = "DIR " if entry.is_directory else "FILE"
        print(f"{kind} {entry.size:>10} bytes  LBA {entry.extent_lba:>8}  {entry.path}")
    print("Read-only metadata listing only; no file extracted, decrypted, or launched.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
