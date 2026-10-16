"""Safety-boundary tests for the deliberately limited PowerPC translator."""

import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.pipeline import translate_program, translate_word


def addi(rt, ra, immediate):
    return (14 << 26) | (rt << 21) | (ra << 16) | (immediate & 0xFFFF)


def branch(displacement, absolute=False, link=False):
    return (
        (18 << 26)
        | ((displacement >> 2 & 0x00FFFFFF) << 2)
        | (int(absolute) << 1)
        | int(link)
    )


def add(rt, ra, rb, record=False):
    return (
        (31 << 26) | (rt << 21) | (ra << 16)
        | (rb << 11) | (266 << 1) | int(record)
    )


class TranslationSafetyTests(unittest.TestCase):
    def test_rejects_absolute_branch(self):
        with self.assertRaisesRegex(ValueError, "Absolute PowerPC branches"):
            translate_word(branch(4, absolute=True))

    def test_rejects_link_branch(self):
        with self.assertRaisesRegex(ValueError, "Linked PowerPC branches"):
            translate_word(branch(4, link=True))

    def test_rejects_gpr31_until_spill_path_exists(self):
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(addi(31, 0, 1))
        with self.assertRaisesRegex(ValueError, "r31"):
            translate_word(add(3, 31, 4))

    def test_rejects_record_form_add(self):
        with self.assertRaisesRegex(ValueError, "record form"):
            translate_word(add(3, 4, 5, record=True))

    def test_program_error_identifies_guest_address(self):
        with self.assertRaisesRegex(ValueError, "0x00001004"):
            translate_program([addi(3, 0, 1), 0xFFFFFFFF], start_address=0x1000)

    def test_program_start_must_be_aligned(self):
        with self.assertRaisesRegex(ValueError, "4-byte aligned"):
            translate_program([addi(3, 0, 1)], start_address=2)


if __name__ == "__main__":
    unittest.main()
