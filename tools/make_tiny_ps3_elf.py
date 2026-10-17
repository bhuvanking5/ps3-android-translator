#!/usr/bin/env python3
"""Generate a tiny, unencrypted ELF64 big-endian PowerPC64 test executable.

This fixture only uses instructions implemented by the prototype guest runner.
It is for testing the experimental ELF loader/runner, not a commercial PS3 game.
"""
from __future__ import annotations

import argparse
import struct
from pathlib import Path

ENTRY = 0x10000
ELF_HEADER_SIZE = 64
PROGRAM_HEADER_SIZE = 56
CODE_OFFSET = ELF_HEADER_SIZE + PROGRAM_HEADER_SIZE

# addi r3,0,7; addi r4,0,5; add r5,r3,r4; b .
WORDS = (0x38600007, 0x38800005, 0x7CA32214, 0x48000000)


def build_elf() -> bytes:
    code = b"".join(struct.pack(">I", word) for word in WORDS)
    image = bytearray(CODE_OFFSET + len(code))
    image[0:16] = b"\x7fELF" + bytes((2, 2, 1, 0)) + bytes(8)

    # ELF64 header: executable, big-endian PowerPC64, entry and one program header.
    struct.pack_into(
        ">HHIQQQIHHHHHH", image, 16,
        2, 21, 1, ENTRY, ELF_HEADER_SIZE, 0, 0,
        ELF_HEADER_SIZE, PROGRAM_HEADER_SIZE, 1, 0, 0, 0,
    )
    # PT_LOAD: read + execute, file-backed code, 4-byte alignment.
    struct.pack_into(
        ">IIQQQQQQ", image, ELF_HEADER_SIZE,
        1, 5, CODE_OFFSET, ENTRY, ENTRY, len(code), len(code), 4,
    )
    image[CODE_OFFSET:] = code
    return bytes(image)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("output", nargs="?", default="tiny_ps3_test.elf",
                        help="output ELF path (default: tiny_ps3_test.elf)")
    args = parser.parse_args()
    output = Path(args.output)
    payload = build_elf()
    output.write_bytes(payload)
    print(f"Wrote {output} ({len(payload)} bytes), entry=0x{ENTRY:08x}")
    print("Expected guest result: r3=7, r4=5, r5=12; then branch in place.")


if __name__ == "__main__":
    main()