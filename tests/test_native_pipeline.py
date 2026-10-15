
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.native_pipeline import (
    translate_word_to_machine_code,
    translate_program_to_machine_code,
)


class NativePipelineTests(unittest.TestCase):

    def test_translate_addi_to_machine_code(self):
        # PowerPC: addi r3, r0, 7
        ppc_word = 0x38600007

        result = translate_word_to_machine_code(ppc_word)

        self.assertEqual(result, bytes.fromhex("e30080d2"))

    def test_translate_add_to_machine_code(self):
        # PowerPC: add r3, r4, r5
        ppc_word = 0x7C642A14

        result = translate_word_to_machine_code(ppc_word)

        self.assertEqual(result, bytes.fromhex("8300058b"))

    def test_translate_multiple_instructions(self):
        instructions = [
            0x38600007,  # addi r3, r0, 7
            0x38830005,  # addi r4, r3, 5
        ]

        result = translate_program_to_machine_code(instructions)

        self.assertEqual(len(result), 8)

    def test_reject_unsupported_instruction(self):
        # Conditional branch opcode 16 is not implemented in native codegen.
        with self.assertRaises(ValueError):
            translate_word_to_machine_code(0x40000004)


if __name__ == "__main__":
    unittest.main()
      
