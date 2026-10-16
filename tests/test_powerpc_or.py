import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.arm64_encoder import encode_instruction
from prototype.cpu import CPUState
from prototype.pipeline import translate_word
from prototype.ppc_decoder import decode_word


def ppc_or(ra, rs, rb, record=False):
    # PowerPC: or rA, rS, rB (opcode 31, XO 444)
    return (
        (31 << 26) | (rs << 21) | (ra << 16)
        | (rb << 11) | (444 << 1) | int(record)
    )


class PowerPCOrTests(unittest.TestCase):
    def test_decoder_fields(self):
        ins = decode_word(ppc_or(3, 4, 5))
        self.assertEqual(ins["mnemonic"], "or")
        self.assertEqual((ins["ra"], ins["rt"], ins["rb"]), (3, 4, 5))
        self.assertFalse(ins["record"])

    def test_reference_cpu_executes_or(self):
        cpu = CPUState()
        cpu.pc = 0x1000
        cpu.gpr[4] = 0b1010
        cpu.gpr[5] = 0b1100
        cpu.execute(ppc_or(3, 4, 5))
        self.assertEqual(cpu.gpr[3], 0b1110)
        self.assertEqual(cpu.pc, 0x1004)

    def test_translation_maps_to_orr(self):
        self.assertEqual(
            translate_word(ppc_or(3, 4, 5)),
            ["    orr x3, x4, x5"],
        )

    def test_encoder_machine_word(self):
        self.assertEqual(
            encode_instruction("orr x3, x4, x5"),
            0xAA050083,
        )

    def test_record_form_is_rejected(self):
        with self.assertRaisesRegex(ValueError, "record form"):
            translate_word(ppc_or(3, 4, 5, record=True))


if __name__ == "__main__":
    unittest.main()
