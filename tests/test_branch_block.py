
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.branch_block import translate_block


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class BranchBlockTests(unittest.TestCase):

    def test_forward_branch(self):
        # Branch from guest address 0x1000 to 0x1008.
        result = translate_block(
            [0x48000008, addi(3, 0, 1), addi(4, 0, 2)],
            start_address=0x1000,
        )
        self.assertEqual(result[:4], bytes.fromhex("02000014"))
        self.assertEqual(len(result), 12)

    def test_backward_branch(self):
        # Branch at guest address 0x1004 back to 0x1000.
        result = translate_block(
            [addi(3, 0, 1), 0x4BFFFFFC],
            start_address=0x1000,
        )
        self.assertEqual(result[-4:], bytes.fromhex("FFFFFF17"))

    def test_branch_mapping_after_expanded_instruction(self):
        # The first guest instruction becomes two ARM64 instructions.
        result = translate_block([
            addi(3, 0, -1),
            0x48000008,
            addi(4, 0, 2),
            addi(5, 0, 3),
        ])
        # The branch is at output offset 8 and targets offset 16.
        self.assertEqual(result[8:12], bytes.fromhex("02000014"))

    def test_reject_branch_target_outside_block(self):
        with self.assertRaisesRegex(ValueError, "outside"):
            translate_block([0x48000008])

    def test_reject_absolute_branch(self):
        with self.assertRaisesRegex(ValueError, "Absolute branch"):
            translate_block([0x4800000A])

    def test_reject_link_branch(self):
        with self.assertRaisesRegex(ValueError, "Link branch"):
            translate_block([0x48000009])

    def test_reject_unaligned_start_address(self):
        with self.assertRaises(ValueError):
            translate_block([addi(3, 0, 1)], start_address=2)


if __name__ == "__main__":
    unittest.main()
  
