
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.control_flow import analyze_branch


class ControlFlowTests(unittest.TestCase):

    def test_relative_forward_branch(self):
        result = analyze_branch(0x48000004, 0x1000)
        self.assertEqual(result["target_address"], 0x1004)

    def test_relative_backward_branch(self):
        result = analyze_branch(0x4BFFFFFC, 0x1000)
        self.assertEqual(result["target_address"], 0x0FFC)

    def test_link_bit_is_preserved(self):
        result = analyze_branch(0x48000005, 0x2000)
        self.assertEqual(result["target_address"], 0x2004)
        self.assertTrue(result["link"])

    def test_rejects_non_branch(self):
        with self.assertRaises(ValueError):
            analyze_branch(0x38600007, 0x1000)

    def test_rejects_unaligned_address(self):
        with self.assertRaises(ValueError):
            analyze_branch(0x48000004, 0x1001)


if __name__ == "__main__":
    unittest.main()
  
