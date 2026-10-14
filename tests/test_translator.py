import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.ppc_to_aarch64 import translate


class TranslatorTests(unittest.TestCase):

    def test_addi(self):
        self.assertEqual(
            translate("addi r3, r4, 10"),
            "    add x3, x4, #10"
        )

    def test_addi_zero_base(self):
        self.assertEqual(
            translate("addi r3, r0, 7"),
            "    add x3, xzr, #7"
        )

    def test_add(self):
        self.assertEqual(
            translate("add r5, r6, r7"),
            "    add x5, x6, x7"
        )

    def test_comment(self):
        self.assertEqual(translate("# just a comment"), "")

    def test_unsupported_instruction(self):
        with self.assertRaises(ValueError):
            translate("unknown r1, r2, r3")


if __name__ == "__main__":
    unittest.main()
