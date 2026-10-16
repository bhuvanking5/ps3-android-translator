import os
import sys
import struct
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from prototype.elf_loader import inspect_ps3_elf, ELF_HEADER, PROGRAM_HEADER


def make_elf(machine=21, elf_class=2, endian=2, truncate=False):
    ident = bytearray(16)
    ident[:4] = b"\x7fELF"
    ident[4] = elf_class
    ident[5] = endian
    ident[6] = 1
    phoff = ELF_HEADER.size
    header = ELF_HEADER.pack(bytes(ident), 2, machine, 1, 0x10000, phoff, 0, 0,
                             ELF_HEADER.size, PROGRAM_HEADER.size, 1, 0, 0, 0)
    ph = PROGRAM_HEADER.pack(1, 5, ELF_HEADER.size + PROGRAM_HEADER.size,
                             0x10000, 0x10000, 4, 8, 4)
    data = header + ph + b"TEST"
    return data[:-2] if truncate else data


class PS3ELFLoaderTests(unittest.TestCase):
    def test_inspects_big_endian_ppc64_elf(self):
        info = inspect_ps3_elf(make_elf())
        self.assertTrue(info["is_executable"])
        self.assertEqual(info["entry_point"], 0x10000)
        self.assertEqual(info["program_header_count"], 1)
        self.assertEqual(info["load_segments"][0]["file_size"], 4)
        self.assertEqual(info["load_segments"][0]["memory_size"], 8)

    def test_rejects_wrong_machine(self):
        with self.assertRaisesRegex(ValueError, "PowerPC64"):
            inspect_ps3_elf(make_elf(machine=62))

    def test_rejects_wrong_endianness(self):
        with self.assertRaisesRegex(ValueError, "big-endian"):
            inspect_ps3_elf(make_elf(endian=1))

    def test_rejects_truncated_segment(self):
        with self.assertRaisesRegex(ValueError, "extends beyond"):
            inspect_ps3_elf(make_elf(truncate=True))

    def test_rejects_non_elf(self):
        with self.assertRaisesRegex(ValueError, "ELF magic"):
            inspect_ps3_elf(b"not an elf file")


if __name__ == "__main__":
    unittest.main()
