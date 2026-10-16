import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.arm64_encoder import encode_program
from prototype.pipeline import translate_program


def ppc_addi(rt, ra, immediate):
    return (
        (14 << 26) | (rt << 21) | (ra << 16)
        | (immediate & 0xFFFF)
    )


def ppc_subf(rt, ra, rb):
    return (
        (31 << 26) | (rt << 21) | (ra << 16)
        | (rb << 11) | (40 << 1)
    )


class TranslationProgramTests(unittest.TestCase):
    def test_native_smoke_sequence_matches_arm64_encoder(self):
        # The Android bridge uses only this fixed, trusted test program:
        # x0 = 7; x1 = 19; x0 = x1 - x0; return x0 (12).
        native_sequence = [
            "mov x0, #7",
            "mov x1, #19",
            "sub x0, x1, x0",
        ]
        machine_code = encode_program(native_sequence)
        words = [
            int.from_bytes(machine_code[i:i + 4], "little")
            for i in range(0, len(machine_code), 4)
        ]
        self.assertEqual(words, [0xD28000E0, 0xD2800261, 0xCB000020])

    def test_multiple_instructions_translate_and_encode_in_order:
        # r3 = 7; r4 = 19; r5 = r4 - r3 = 12.
        guest_program = [
            ppc_addi(3, 0, 7),
            ppc_addi(4, 0, 19),
            ppc_subf(5, 3, 4),
        ]
        translated = translate_program(guest_program, start_address=0x1000)

        self.assertEqual(
            translated,
            [
                "    mov x3, #7",
                "    mov x4, #19",
                "    sub x5, x4, x3",
            ],
        )
        machine_code = encode_program(translated)
        self.assertEqual(len(machine_code), 12)
        self.assertEqual(
            [int.from_bytes(machine_code[i:i + 4], "little")
             for i in range(0, len(machine_code), 4)],
            [0xD28000E3, 0xD2800264, 0xCB030085],
        )


if __name__ == "__main__":
    unittest.main()
