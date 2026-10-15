
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.validation import validate_block


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class EndToEndTests(unittest.TestCase):

    def test_translate_complete_block(self):
        # addi r3, r0, 7
        # b to the next instruction
        # addi r4, r0, 2
        report = validate_block([
            addi(3, 0, 7),
            0x48000004,
            addi(4, 0, 2),
        ])

        self.assertEqual(report["status"], "passed")
        self.assertEqual(report["guest_instruction_count"], 3)
        self.assertEqual(report["arm64_instruction_count"], 3)
        self.assertEqual(report["machine_code_size"], 12)
        self.assertEqual(
            report["machine_code_hex"],
            "e30080d201000014440080d2",
        )

    def test_expanded_instruction_changes_output_count(self):
        # addi r3, r0, -1 becomes two ARM64 instructions.
        report = validate_block([
            addi(3, 0, -1),
            addi(4, 0, 2),
        ])

        self.assertEqual(report["guest_instruction_count"], 2)
        self.assertEqual(report["arm64_instruction_count"], 3)
        self.assertEqual(report["machine_code_size"], 12)

    def test_reject_empty_block(self):
        with self.assertRaisesRegex(ValueError, "must not be empty"):
            validate_block([])

    def test_reject_unsupported_instruction(self):
        with self.assertRaises(ValueError):
            validate_block([0xFFFFFFFF])


if __name__ == "__main__":
    unittest.main()
      
