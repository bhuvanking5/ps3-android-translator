import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from prototype.basic_block import build_basic_blocks


def addi(rt, ra, immediate):
    return (14 << 26) | (rt << 21) | (ra << 16) | (immediate & 0xFFFF)


def branch(displacement, absolute=False, link=False):
    return (
        (18 << 26)
        | (((displacement >> 2) & 0x00FFFFFF) << 2)
        | (int(absolute) << 1)
        | int(link)
    )


def conditional_branch(displacement=4):
    return (16 << 26) | (((displacement >> 2) & 0x3FFF) << 2)


class BasicBlockTests(unittest.TestCase):
    def test_linear_program_is_one_block(self):
        blocks = build_basic_blocks([addi(3, 0, 1), addi(4, 3, 2)])
        self.assertEqual(len(blocks), 1)
        self.assertEqual(blocks[0]["start_address"], 0)
        self.assertEqual(blocks[0]["terminator"], "fallthrough")
        self.assertEqual(blocks[0]["successors"], [])
        self.assertEqual(len(blocks[0]["arm64"]), 2)

    def test_forward_branch_creates_two_blocks_and_edge(self):
        program = [addi(3, 0, 1), branch(8), addi(4, 0, 2), addi(5, 0, 3)]
        blocks = build_basic_blocks(program, start_address=0x1000)
        self.assertEqual([b["start_address"] for b in blocks], [0x1000, 0x1008, 0x100C])
        self.assertEqual(blocks[0]["terminator"], "branch")
        self.assertEqual(blocks[0]["successors"], [0x100C])
        self.assertEqual(blocks[1]["successors"], [0x100C])
        self.assertEqual(blocks[2]["successors"], [])

    def test_backward_branch_creates_loop_edge(self):
        program = [addi(3, 0, 1), addi(4, 0, 2), branch(-4)]
        blocks = build_basic_blocks(program, start_address=0x2000)
        self.assertEqual(blocks[-1]["successors"], [0x2004])

    def test_rejects_branch_outside_sequence(self):
        with self.assertRaisesRegex(ValueError, "outside this instruction sequence"):
            build_basic_blocks([branch(16), addi(3, 0, 1)])

    def test_rejects_absolute_branch(self):
        with self.assertRaisesRegex(ValueError, "Absolute branch"):
            build_basic_blocks([branch(4, absolute=True), addi(3, 0, 1)])

    def test_rejects_linked_branch(self):
        with self.assertRaisesRegex(ValueError, "Linked branch"):
            build_basic_blocks([branch(4, link=True), addi(3, 0, 1)])

    def test_rejects_conditional_branch(self):
        with self.assertRaisesRegex(ValueError, "conditional branch"):
            build_basic_blocks([conditional_branch(), addi(3, 0, 1)])

    def test_rejects_empty_or_oversized_program(self):
        with self.assertRaisesRegex(ValueError, "must not be empty"):
            build_basic_blocks([])
        with self.assertRaisesRegex(ValueError, "exceeds 256"):
            build_basic_blocks([addi(3, 0, 1)] * 257)

    def test_rejects_bad_start_address(self):
        with self.assertRaisesRegex(ValueError, "aligned"):
            build_basic_blocks([addi(3, 0, 1)], start_address=2)


if __name__ == "__main__":
    unittest.main()
