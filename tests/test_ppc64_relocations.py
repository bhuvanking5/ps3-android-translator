import unittest

from tools.ppc64_relocations import (
    R_PPC64_ADDR32,
    R_PPC64_ADDR64,
    R_PPC64_REL64,
    RelocationError,
    apply_rela,
)


class PPC64RelocationTests(unittest.TestCase):
    def test_addr32_writes_big_endian_s_plus_a(self):
        memory = bytearray(8)
        apply_rela(memory, 2, R_PPC64_ADDR32, 0x1000, 0x24, 0)
        self.assertEqual(memory, b"\x00\x00\x00\x00\x10\x24\x00\x00")

    def test_addr64_writes_big_endian_s_plus_a(self):
        memory = bytearray(8)
        apply_rela(memory, 0, R_PPC64_ADDR64, 0x123400000000, 0x55, 0)
        self.assertEqual(memory, bytes.fromhex("0000123400000055"))

    def test_rel64_writes_signed_s_plus_a_minus_p(self):
        memory = bytearray(8)
        apply_rela(memory, 0, R_PPC64_REL64, 0x1000, -8, 0x1010)
        self.assertEqual(int.from_bytes(memory, "big", signed=True), -0x18)

    def test_rejects_unsupported_type_without_mutating_memory(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "unsupported"):
            apply_rela(memory, 0, 999, 1, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_out_of_bounds_target_without_mutation(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "outside"):
            apply_rela(memory, 6, R_PPC64_ADDR32, 1, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_unsigned_overflow_without_mutation(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "out of range"):
            apply_rela(memory, 0, R_PPC64_ADDR32, 1 << 32, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_negative_absolute_result(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "out of range"):
            apply_rela(memory, 0, R_PPC64_ADDR64, 0, -1, 0)
        self.assertEqual(memory, b"abcdefgh")


if __name__ == "__main__":
    unittest.main()
