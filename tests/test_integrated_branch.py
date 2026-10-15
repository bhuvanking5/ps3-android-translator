
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.integrated_engine import execute_and_validate


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class IntegratedBranchTests(unittest.TestCase):

    def test_branch_skips_instruction_and_translation_passes(self):
        report = execute_and_validate([
            addi(3, 0, 1),  # r3 = 1
            0x48000008,     # Jump forward by 8 bytes
            addi(3, 0, 99), # Must be skipped
            addi(4, 0, 7),  # r4 = 7
        ])

        self.assertEqual(report["status"], "passed")
        self.assertEqual(report["execution_steps"], 3)
        self.assertEqual(report["registers"]["r3"], 1)
        self.assertEqual(report["registers"]["r4"], 7)
        self.assertEqual(
            report["translation"]["guest_instruction_count"], 4
        )
        self.assertFalse(report["native_execution"])



    def test_conditional_branch_is_not_claimed_as_native_translation(self):
        # bc: BO=16 ignores CR and branches while decremented CTR != 0.
        # The reference CPU supports this instruction, but the ARM64
        # translator does not yet implement conditional branch translation.
        with self.assertRaises(ValueError):
            execute_and_validate([
                0x42000004,  # Conditional branch instruction
                addi(3, 0, 7),
            ])


if __name__ == "__main__":
    unittest.main()
