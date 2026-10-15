
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


class IntegratedEngineTests(unittest.TestCase):

    def test_executes_and_translates_program(self):
        # addi r3, r0, 7
        # addi r4, r0, 2
        report = execute_and_validate([
            addi(3, 0, 7),
            addi(4, 0, 2),
        ])

        self.assertEqual(report["status"], "passed")
        self.assertEqual(report["execution_steps"], 2)
        self.assertEqual(report["final_pc"], 8)
        self.assertEqual(report["registers"]["r3"], 7)
        self.assertEqual(report["registers"]["r4"], 2)
        self.assertEqual(
            report["translation"]["guest_instruction_count"], 2
        )
        self.assertTrue(
            report["translation"]["machine_code_hex"]
        )
        self.assertFalse(report["native_execution"])

    def test_rejects_empty_program(self):
        with self.assertRaisesRegex(ValueError, "must not be empty"):
            execute_and_validate([])


if __name__ == "__main__":
    unittest.main()
      
