
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.cpu import CPUState
from prototype.program import run_program


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class ProgramBranchTests(unittest.TestCase):

    def test_forward_branch_skips_instruction(self):
        cpu = CPUState()

        instructions = [
            addi(3, 0, 1),   # r3 = 1
            0x48000008,      # Jump forward by 8 bytes
            addi(3, 0, 99),  # Must be skipped
            addi(4, 0, 7),   # r4 = 7
        ]

        steps = run_program(cpu, instructions)

        self.assertEqual(cpu.gpr[3], 1)
        self.assertEqual(cpu.gpr[4], 7)
        self.assertEqual(steps, 3)

    def test_backward_branch_hits_step_limit(self):
        cpu = CPUState()

        # Branch back to itself forever.
        with self.assertRaisesRegex(
            RuntimeError, "Maximum instruction step count"
        ):
            run_program(cpu, [0x48000000], max_steps=5)


if __name__ == "__main__":
    unittest.main()
