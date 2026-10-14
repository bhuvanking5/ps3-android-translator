
import os
import sys
import unittest

sys.path.insert(
    0, os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.code_cache import TranslationCache


class CodeCacheTests(unittest.TestCase):

    def test_first_request_is_cache_miss(self):
        cache = TranslationCache()
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10

        result = cache.get_or_translate(word)

        self.assertEqual(result, ["    add x3, x4, #10"])
        self.assertEqual(cache.misses, 1)
        self.assertEqual(cache.hits, 0)
        self.assertEqual(cache.size, 1)

    def test_repeated_request_is_cache_hit(self):
        calls = []

        def fake_translator(word):
            calls.append(word)
            return ["    test instruction"]

        cache = TranslationCache(translator=fake_translator)

        first = cache.get_or_translate(123)
        second = cache.get_or_translate(123)

        self.assertEqual(first, second)
        self.assertEqual(len(calls), 1)
        self.assertEqual(cache.hits, 1)
        self.assertEqual(cache.misses, 1)

    def test_different_instructions_have_separate_entries(self):
        cache = TranslationCache()
        cache.get_or_translate(
            (14 << 26) | (3 << 21) | (4 << 16) | 10
        )
        cache.get_or_translate(
            (14 << 26) | (5 << 21) | (6 << 16) | 20
        )

        self.assertEqual(cache.size, 2)
        self.assertEqual(cache.misses, 2)

    def test_clear_resets_cache_and_counters(self):
        cache = TranslationCache()
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10

        cache.get_or_translate(word)
        cache.get_or_translate(word)
        cache.clear()

        self.assertEqual(cache.size, 0)
        self.assertEqual(cache.hits, 0)
        self.assertEqual(cache.misses, 0)

    def test_failed_translation_is_not_cached(self):
        calls = []

        def failing_translator(word):
            calls.append(word)
            raise ValueError("Unsupported instruction")

        cache = TranslationCache(translator=failing_translator)

        for _ in range(2):
            with self.assertRaises(ValueError):
                cache.get_or_translate(999)

        self.assertEqual(len(calls), 2)
        self.assertEqual(cache.size, 0)

    def test_returned_list_cannot_modify_cached_result(self):
        cache = TranslationCache()
        word = (14 << 26) | (3 << 21) | (4 << 16) | 10

        first = cache.get_or_translate(word)
        first.append("corrupted")

        second = cache.get_or_translate(word)
        self.assertEqual(second, ["    add x3, x4, #10"])


if __name__ == "__main__":
    unittest.main()
      
