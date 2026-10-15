import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.pipeline import translate_word, translate_bytes


class PipelineTests(unittest.TestCase):

    def test_addi_positive(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        self.assertEqual(
            translate_word(word), ["    add x3, x4, #10"]
        )

    def test_addi_negative(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 0xFFFC
        self.assertEqual(
            translate_word(word), ["    sub x3, x4, #4"]
        )

    def test_addi_zero_base(self):
        word = (14 << 26) | (3 << 21) | 7
        self.assertEqual(
            translate_word(word), ["    mov x3, #7"]
        )

    def test_add_instruction(self):
        word = (
            (31 << 26) | (3 << 21) | (4 << 16)
            | (5 << 11) | (266 << 1)
        )
        self.assertEqual(
            translate_word(word), ["    add x3, x4, x5"]
        )

    def test_translate_big_endian_bytes(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        self.assertEqual(
            translate_bytes(word.to_bytes(4, "big")),
            ["    add x3, x4, #10"]
        )

    def test_reject_wrong_byte_length(self):
        with self.assertRaises(ValueError):
            translate_bytes(b"\\x00\\x00")

    def test_reject_add_record_form(self):
        word = (
            (31 << 26) | (3 << 21) | (4 << 16)
            | (5 << 11) | (266 << 1) | 1
        )
        with self.assertRaises(ValueError):
            translate_word(word)

    def test_reject_r31_destination(self):
        word = (14 << 26) | (31 << 21) | 7
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(word)

    def test_reject_r31_source(self):
        word = (14 << 26) | (3 << 21) | (31 << 16) | 7
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(word)

    def test_reject_r31_add_operand(self):
        word = (
            (31 << 26) | (3 << 21) | (4 << 16)
            | (31 << 11) | (266 << 1)
        )
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(word)


if __name__ == "__main__":
    unittest.main()
