
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.cpu import CPUState


def make_addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


def make_add(rt, ra, rb):
    return (
        (31 << 26)
        | (rt << 21)
        | (ra << 16)
        | (rb << 11)
        | (266 << 1)
    )


def make_conditional_branch(bo, bi, displacement, absolute=False, link=False):
    return (
        (16 << 26)
        | (bo << 21)
        | (bi << 16)
        | (((displacement >> 2) & 0x3FFF) << 2)
        | (int(absolute) << 1)
        | int(link)
    )


def make_branch(displacement, absolute=False, link=False):
    encoded = (displacement >> 2) & 0x00FFFFFF
    return (
        (18 << 26)
        | (encoded << 2)
        | (int(absolute) << 1)
        | int(link)
    )


class CPUTests(unittest.TestCase):

    def test_addi(self):
        cpu = CPUState()
        cpu.gpr[4] = 20

        cpu.execute(make_addi(3, 4, 7))

        self.assertEqual(cpu.gpr[3], 27)
        self.assertEqual(cpu.pc, 4)

    def test_addi_uses_zero_when_base_is_zero(self):
        cpu = CPUState()

        cpu.execute(make_addi(3, 0, 12))

        self.assertEqual(cpu.gpr[3], 12)

    def test_negative_addi(self):
        cpu = CPUState()
        cpu.gpr[4] = 10

        cpu.execute(make_addi(3, 4, -4))

        self.assertEqual(cpu.gpr[3], 6)

    def test_add(self):
        cpu = CPUState()
        cpu.gpr[4] = 15
        cpu.gpr[5] = 9

        cpu.execute(make_add(3, 4, 5))

        self.assertEqual(cpu.gpr[3], 24)

    def test_relative_branch(self):
        cpu = CPUState()
        cpu.pc = 100

        cpu.execute(make_branch(16))

        self.assertEqual(cpu.pc, 116)

    def test_absolute_branch(self):
        cpu = CPUState()

        cpu.execute(make_branch(40, absolute=True))

        self.assertEqual(cpu.pc, 40)

    def test_branch_link(self):
        cpu = CPUState()
        cpu.pc = 100

        cpu.execute(make_branch(16, link=True))

        self.assertEqual(cpu.lr, 104)
        self.assertEqual(cpu.pc, 116)

    def test_conditional_branch_tests_cr_bit_true(self):
        cpu = CPUState()
        cpu.pc = 100
        cpu.cr = 1 << (31 - 2)
        cpu.execute(make_conditional_branch(12, 2, 16))
        self.assertEqual(cpu.pc, 116)

    def test_conditional_branch_not_taken_advances_pc(self):
        cpu = CPUState()
        cpu.pc = 100
        cpu.execute(make_conditional_branch(12, 2, 16))
        self.assertEqual(cpu.pc, 104)

    def test_conditional_branch_ctr_decrement(self):
        cpu = CPUState()
        cpu.pc = 100
        cpu.ctr = 2
        # BO=16 ignores CR and branches while decremented CTR != 0.
        cpu.execute(make_conditional_branch(16, 0, 16))
        self.assertEqual(cpu.ctr, 1)
        self.assertEqual(cpu.pc, 116)

    def test_unsupported_instruction_does_not_advance_pc(self):
        cpu = CPUState()

        with self.assertRaises(ValueError):
            cpu.execute(0)

        self.assertEqual(cpu.pc, 0)


if __name__ == "__main__":
    unittest.main()
