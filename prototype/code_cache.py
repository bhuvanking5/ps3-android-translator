
"""Small cache for previously translated instructions."""

from prototype.pipeline import translate_word


class TranslationCache:
    def __init__(self, translator=None):
        self.translator = translator or translate_word
        self._cache = {}
        self.hits = 0
        self.misses = 0

    def get_or_translate(self, word):
        """Return cached ARM64 lines, or translate and cache them."""
        if word in self._cache:
            self.hits += 1
            return list(self._cache[word])

        self.misses += 1
        result = self.translator(word)

        # Store only successful translations.
        self._cache[word] = tuple(result)
        return list(result)

    def clear(self):
        """Remove cached code and reset performance counters."""
        self._cache.clear()
        self.hits = 0
        self.misses = 0

    @property
    def size(self):
        return len(self._cache)
      
