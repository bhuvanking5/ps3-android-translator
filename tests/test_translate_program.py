
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.pipeline import translate_program


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class TranslateProgramTests(unittest.TestCase):

    def test_translates_multiple_instructions(self):
        program = [
            addi(3, 0, 10),
            addi(4, 3, 5),
        ]

        result = translate_program(program)

        self.assertEqual(
            result,
            [
                "    mov x3, #10",
                "    add x4, x3, #5",
            ],
        )

    def test_empty_program(self):
        self.assertEqual(translate_program([]), [])

    def test_rejects_unaligned_start_address(self):
        with self.assertRaises(ValueError):
            translate_program(
                [addi(3, 0, 10)],
                start_address=2,
            )

    def test_reports_unsupported_instruction_address(self):
        with self.assertRaisesRegex(ValueError, "0x00001004"):
            translate_program(
                [addi(3, 0, 10), 0x00000000],
                start_address=0x1000,
            )


if __name__ == "__main__":
    unittest.main()
      
