
import os
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from prototype.ppc_decoder import decode_word, decode_bytes


class DecoderTests(unittest.TestCase):

    def test_decode_addi(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        result = decode_word(word)
        self.assertEqual(result["mnemonic"], "addi")
        self.assertEqual(result["rt"], 3)
        self.assertEqual(result["ra"], 4)
        self.assertEqual(result["immediate"], 10)

    def test_decode_negative_immediate(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 0xFFFC
        self.assertEqual(decode_word(word)["immediate"], -4)

    def test_decode_add(self):
        word = (31 << 26) | (3 << 21) | (4 << 16) | (5 << 11) | (266 << 1)
        result = decode_word(word)
        self.assertEqual(result["mnemonic"], "add")
        self.assertEqual(result["rt"], 3)
        self.assertEqual(result["ra"], 4)
        self.assertEqual(result["rb"], 5)
        self.assertFalse(result["record"])

    def test_decode_conditional_branch_fields(self):
        # bc BO=12, BI=3, BD=+16, AA=0, LK=0
        word = (16 << 26) | (12 << 21) | (3 << 16) | (16 & 0xFFFC)
        result = decode_word(word)
        self.assertEqual(result["mnemonic"], "bc")
        self.assertEqual(result["bo"], 12)
        self.assertEqual(result["bi"], 3)
        self.assertEqual(result["displacement"], 16)
        self.assertFalse(result["absolute"])
        self.assertFalse(result["link"])

    def test_decode_conditional_branch_negative_displacement(self):
        # BD is a signed 14-bit word displacement.
        encoded_bd = (-16 >> 2) & 0x3FFF
        word = (16 << 26) | (encoded_bd << 2)
        self.assertEqual(decode_word(word)["displacement"], -16)

    def test_decode_conditional_branch_flags(self):
        result = decode_word((16 << 26) | 3)
        self.assertTrue(result["absolute"])
        self.assertTrue(result["link"])

    def test_decode_branch_positive_displacement(self):
        result = decode_word((18 << 26) | 16)
        self.assertEqual(result["mnemonic"], "b")
        self.assertEqual(result["displacement"], 16)
        self.assertFalse(result["absolute"])
        self.assertFalse(result["link"])

    def test_decode_branch_negative_displacement(self):
        encoded_li = (-16 >> 2) & 0x00FFFFFF
        result = decode_word((18 << 26) | (encoded_li << 2))
        self.assertEqual(result["displacement"], -16)

    def test_decode_branch_absolute_and_link_flags(self):
        result = decode_word((18 << 26) | 0x3)
        self.assertEqual(result["displacement"], 0)
        self.assertTrue(result["absolute"])
        self.assertTrue(result["link"])

    def test_decode_big_endian_bytes(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        self.assertEqual(decode_bytes(word.to_bytes(4, "big"))["immediate"], 10)

    def test_decode_branch_from_bytes(self):
        result = decode_bytes(((18 << 26) | 16).to_bytes(4, "big"))
        self.assertEqual(result["mnemonic"], "b")
        self.assertEqual(result["displacement"], 16)

    def test_wrong_byte_length(self):
        with self.assertRaises(ValueError):
            decode_bytes(b"\\x00\\x00")

    def test_wrong_byte_type(self):
        with self.assertRaises(ValueError):
            decode_bytes("abcd")

    def test_unsupported_instruction(self):
        with self.assertRaises(ValueError):
            decode_word(0)

    def test_invalid_word_range(self):
        with self.assertRaises(ValueError):
            decode_word(0x100000000)

    def test_negative_word(self):
        with self.assertRaises(ValueError):
            decode_word(-1)

    def test_non_integer_word(self):
        with self.assertRaises(ValueError):
            decode_word("123")

    def test_boolean_is_not_an_instruction(self):
        with self.assertRaises(ValueError):
            decode_word(True)


if __name__ == "__main__":
    unittest.main()
