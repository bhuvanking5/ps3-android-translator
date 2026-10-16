import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.pipeline import translate_word, translate_bytes


def x_form(rs, ra, rb, xo, record=False):
    """Build a PowerPC opcode-31 instruction for translator unit tests."""
    return (
        (31 << 26) | (rs << 21) | (ra << 16) | (rb << 11)
        | (xo << 1) | int(record)
    )


class PipelineTests(unittest.TestCase):

    def test_addi_positive(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        self.assertEqual(translate_word(word), ["    add x3, x4, #10"])

    def test_addi_negative(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 0xFFFC
        self.assertEqual(translate_word(word), ["    sub x3, x4, #4"])

    def test_addi_zero_base(self):
        word = (14 << 26) | (3 << 21) | 7
        self.assertEqual(translate_word(word), ["    mov x3, #7"])

    def test_add_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 266)),
            ["    add x3, x4, x5"],
        )

    def test_or_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 444)),
            ["    orr x4, x3, x5"],
        )

    def test_and_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 28)),
            ["    and x4, x3, x5"],
        )

    def test_xor_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 316)),
            ["    eor x4, x3, x5"],
        )

    def test_subf_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 40)),
            ["    sub x3, x5, x4"],
        )

    def test_mulld_instruction(self):
        self.assertEqual(
            translate_word(x_form(3, 4, 5, 233)),
            ["    mul x3, x4, x5"],
        )

    def test_relative_unlinked_branch_forward(self):
        word = (18 << 26) | (16 & 0x03FFFFFC)
        self.assertEqual(translate_word(word), ["    b #16"])

    def test_relative_unlinked_branch_backward(self):
        li = (-16 >> 2) & 0x00FFFFFF
        word = (18 << 26) | (li << 2)
        self.assertEqual(translate_word(word), ["    b #-16"])

    def test_reject_linked_branch(self):
        word = (18 << 26) | (16 & 0x03FFFFFC) | 1
        with self.assertRaisesRegex(ValueError, "Linked"):
            translate_word(word)

    def test_reject_absolute_branch(self):
        word = (18 << 26) | (16 & 0x03FFFFFC) | 2
        with self.assertRaisesRegex(ValueError, "Absolute"):
            translate_word(word)

    def test_translate_big_endian_bytes(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        self.assertEqual(
            translate_bytes(word.to_bytes(4, "big")),
            ["    add x3, x4, #10"],
        )

    def test_reject_wrong_byte_length(self):
        with self.assertRaises(ValueError):
            translate_bytes(b"\\x00\\x00")

    def test_reject_add_record_form(self):
        with self.assertRaises(ValueError):
            translate_word(x_form(3, 4, 5, 266, record=True))

    def test_reject_logical_record_forms(self):
        for xo in (444, 28, 316):
            with self.subTest(xo=xo):
                with self.assertRaisesRegex(ValueError, "record form"):
                    translate_word(x_form(3, 4, 5, xo, record=True))

    def test_reject_r31_destination(self):
        word = (14 << 26) | (31 << 21) | 7
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(word)

    def test_reject_r31_source(self):
        word = (14 << 26) | (3 << 21) | (31 << 16) | 7
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(word)

    def test_reject_r31_add_operand(self):
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(x_form(3, 4, 31, 266))

    def test_reject_r31_logical_source(self):
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(x_form(31, 4, 5, 28))


if __name__ == "__main__":
    unittest.main()
