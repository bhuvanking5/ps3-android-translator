import struct
import unittest

from tools.make_tiny_ps3_elf import CODE_OFFSET, ENTRY, WORDS, build_elf


class TinyElfFixtureTests(unittest.TestCase):
    def test_is_big_endian_elf64_powerpc_executable(self):
        elf = build_elf()
        self.assertEqual(elf[:4], b"\x7fELF")
        self.assertEqual(elf[4:7], bytes((2, 2, 1)))
        self.assertEqual(struct.unpack_from(">HHI", elf, 16), (2, 21, 1))
        self.assertEqual(struct.unpack_from(">Q", elf, 24)[0], ENTRY)
        self.assertEqual(struct.unpack_from(">Q", elf, 32)[0], 64)
        self.assertEqual(struct.unpack_from(">HHH", elf, 52), (64, 56, 1))

    def test_load_segment_and_supported_instruction_program(self):
        elf = build_elf()
        ph = struct.unpack_from(">IIQQQQQQ", elf, 64)
        self.assertEqual(ph, (1, 5, CODE_OFFSET, ENTRY, ENTRY, 16, 16, 4))
        words = struct.unpack_from(">IIII", elf, CODE_OFFSET)
        self.assertEqual(words, WORDS)
        self.assertEqual(len(elf), CODE_OFFSET + 16)


if __name__ == "__main__":
    unittest.main()