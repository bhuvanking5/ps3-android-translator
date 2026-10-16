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


def ppc_subf(rt, ra, rb, record=False):
    # PowerPC subf rT, rA, rB computes rT = rB - rA.
    return (
        (31 << 26) | (rt << 21) | (ra << 16)
        | (rb << 11) | (40 << 1) | int(record)
    )


class PowerPCSubfTests(unittest.TestCase):
    def test_decoder_fields(self):
        ins = decode_word(ppc_subf(3, 4, 5))
        self.assertEqual(ins["mnemonic"], "subf")
        self.assertEqual((ins["rt"], ins["ra"], ins["rb"]), (3, 4, 5))
        self.assertFalse(ins["record"])

    def test_reference_cpu_executes_subf(self):
        cpu = CPUState()
        cpu.pc = 0x1000
        cpu.gpr[4] = 7
        cpu.gpr[5] = 19
        cpu.execute(ppc_subf(3, 4, 5))
        self.assertEqual(cpu.gpr[3], 12)
        self.assertEqual(cpu.pc, 0x1004)

    def test_translation_reverses_subtrahends_correctly(self):
        self.assertEqual(
            translate_word(ppc_subf(3, 4, 5)),
            ["    sub x3, x5, x4"],
        )

    def test_encoder_machine_word(self):
        self.assertEqual(
            encode_instruction("sub x3, x5, x4"),
            0xCB0400A3,
        )

    def test_record_form_is_rejected(self):
        cpu = CPUState()
        with self.assertRaisesRegex(ValueError, "record form"):
            cpu.execute(ppc_subf(3, 4, 5, record=True))
        with self.assertRaisesRegex(ValueError, "record form"):
            translate_word(ppc_subf(3, 4, 5, record=True))


if __name__ == "__main__":
    unittest.main()
