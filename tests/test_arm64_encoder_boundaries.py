import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.arm64_encoder import encode_instruction, encode_program


class ARM64EncoderBoundaryTests(unittest.TestCase):

    def test_add_immediate_zero(self):
        self.assertEqual(encode_instruction("add x0, x0, #0"), 0x91000000)

    def test_add_immediate_maximum(self):
        self.assertEqual(
            encode_instruction("add x30, x29, #4095"),
            0x913FFFBE,
        )

    def test_mov_immediate_maximum(self):
        self.assertEqual(
            encode_instruction("mov x30, #65535"),
            0xD29FFFFE,
        )

    def test_branch_minimum_aligned_offset(self):
        self.assertEqual(
            encode_instruction("b #-134217728"),
            0x16000000,
        )

    def test_branch_maximum_aligned_offset(self):
        self.assertEqual(
            encode_instruction("b #134217724"),
            0x15FFFFFF,
        )

    def test_reject_negative_add_immediate(self):
        with self.assertRaises(ValueError):
            encode_instruction("add x1, x2, #-1")

    def test_reject_negative_mov_immediate(self):
        with self.assertRaises(ValueError):
            encode_instruction("mov x1, #-1")

    def test_encode_program_reports_instruction_index(self):
        with self.assertRaisesRegex(ValueError, "instruction 1"):
            encode_program(["mov x0, #1", "nonsense x0"])


if __name__ == "__main__":
    unittest.main()
