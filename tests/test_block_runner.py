
import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.block_runner import run_block
from prototype.code_cache import TranslationCache


def make_addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


class BlockRunnerTests(unittest.TestCase):

    def test_translates_multiple_instructions(self):
        words = [
            make_addi(3, 4, 10),
            make_addi(5, 6, 20),
        ]

        result = run_block(words)

        self.assertIsNone(result["error"])
        self.assertEqual(len(result["translated"]), 2)
        self.assertEqual(
            result["translated"][0], ["    add x3, x4, #10"]
        )
        self.assertEqual(
            result["translated"][1], ["    add x5, x6, #20"]
        )

    def test_reuses_shared_cache(self):
        cache = TranslationCache()
        word = make_addi(3, 4, 10)

        result = run_block([word, word], cache)

        self.assertIsNone(result["error"])
        self.assertEqual(cache.misses, 1)
        self.assertEqual(cache.hits, 1)

    def test_stops_safely_on_unsupported_instruction(self):
        words = [make_addi(3, 4, 10), 0]

        result = run_block(words)

        self.assertEqual(len(result["translated"]), 1)
        self.assertEqual(result["error"]["index"], 1)
        self.assertEqual(result["error"]["word"], 0)

    def test_empty_block(self):
        result = run_block([])

        self.assertEqual(result["translated"], [])
        self.assertIsNone(result["error"])

    def test_rejects_wrong_input_type(self):
        with self.assertRaises(ValueError):
            run_block("not a list of instructions")

    def test_does_not_cache_failed_instruction(self):
        cache = TranslationCache()
        result = run_block([0], cache)

        self.assertIsNotNone(result["error"])
        self.assertEqual(cache.size, 0)


if __name__ == "__main__":
    unittest.main()
      
