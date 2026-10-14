
import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

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
        word = (
            (31 << 26)
            | (3 << 21)
            | (4 << 16)
            | (5 << 11)
            | (266 << 1)
        )
        result = decode_word(word)

        self.assertEqual(result["mnemonic"], "add")
        self.assertEqual(result["rt"], 3)
        self.assertEqual(result["ra"], 4)
        self.assertEqual(result["rb"], 5)
        self.assertFalse(result["record"])

    def test_decode_big_endian_bytes(self):
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10
        result = decode_bytes(word.to_bytes(4, "big"))
        self.assertEqual(result["immediate"], 10)

    def test_wrong_byte_length(self):
        with self.assertRaises(ValueError):
            decode_bytes(b"\x00\x00")

    def test_unsupported_instruction(self):
        with self.assertRaises(ValueError):
            decode_word(0)

    def test_invalid_word_range(self):
        with self.assertRaises(ValueError):
            decode_word(0x100000000)


if __name__ == "__main__":
    unittest.main()
      
