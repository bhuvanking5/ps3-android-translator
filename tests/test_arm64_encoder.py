
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.arm64_encoder import (
    encode_instruction,
    encode_bytes,
    encode_program,
)


class ARM64EncoderTests(unittest.TestCase):

    def test_add_immediate(self):
        self.assertEqual(
            encode_instruction("add x3, x4, #10"),
            0x91002883,
        )

    def test_sub_immediate(self):
        self.assertEqual(
            encode_instruction("sub x3, x3, #4"),
            0xD1001063,
        )

    def test_add_registers(self):
        self.assertEqual(
            encode_instruction("add x3, x4, x5"),
            0x8B050083,
        )

    def test_mov_immediate(self):
        self.assertEqual(
            encode_instruction("mov x3, #7"),
            0xD28000E3,
        )

    def test_mov_register(self):
        self.assertEqual(
            encode_instruction("mov x3, x4"),
            0xAA0403E3,
        )

    def test_bytes_are_little_endian(self):
        self.assertEqual(
            encode_bytes("add x3, x4, #10"),
            bytes.fromhex("83280091"),
        )

    def test_encode_program(self):
        result = encode_program([
            "mov x3, #10",
            "add x4, x3, #5",
        ])
        self.assertEqual(len(result), 8)

    def test_mov_immediate_boundaries(self):
        self.assertEqual(encode_instruction("mov x0, #0"), 0xD2800000)
        self.assertEqual(
            encode_instruction("mov x30, #65535"),
            0xD29FFFE0,
        )

    def test_reject_invalid_assembly_input_types(self):
        for assembly in (None, 7, b"mov x0, #1"):
            with self.subTest(assembly=assembly):
                with self.assertRaisesRegex(
                    ValueError, "must be a string"
                ):
                    encode_instruction(assembly)

    def test_program_encoding_rejects_non_sequence(self):
        for instructions in (None, "mov x0, #1", 3):
            with self.subTest(instructions=instructions):
                with self.assertRaisesRegex(
                    ValueError, "list or tuple"
                ):
                    encode_program(instructions)

    def test_reject_invalid_register(self):
        with self.assertRaises(ValueError):
            encode_instruction("add x31, x4, #10")

    def test_reject_out_of_range_immediate(self):
        with self.assertRaises(ValueError):
            encode_instruction("add x3, x4, #5000")

    def test_reject_unsupported_instruction(self):
        with self.assertRaises(ValueError):
            encode_instruction("BL #100")

    def test_forward_branch(self):
        self.assertEqual(
            encode_instruction("b #16"),
            0x14000004,
        )

    def test_backward_branch(self):
        self.assertEqual(
            encode_instruction("b #-4"),
            0x17FFFFFF,
        )

    def test_branch_bytes(self):
        self.assertEqual(
            encode_bytes("b #16"),
            bytes.fromhex("04000014"),
        )

    def test_reject_unaligned_branch(self):
        with self.assertRaises(ValueError):
            encode_instruction("b #2")

    def test_branch_signed_range_boundaries(self):
        self.assertEqual(
            encode_instruction("b #-134217728"),
            0x16000000,
        )
        self.assertEqual(
            encode_instruction("b #134217724"),
            0x15FFFFFF,
        )

    def test_reject_branch_below_minimum_offset(self):
        with self.assertRaises(ValueError):
            encode_instruction("b #-134217732")

    def test_reject_out_of_range_branch(self):
        with self.assertRaises(ValueError):
            encode_instruction("b #134217728")


if __name__ == "__main__":
    unittest.main()
            
