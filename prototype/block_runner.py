
"""Run a sequence of instructions through the translation cache."""

from prototype.code_cache import TranslationCache


def run_block(words, cache=None):
    """Translate instruction words in order and report any failure."""
    if not isinstance(words, (list, tuple)):
        raise ValueError("words must be a list or tuple of instruction words")

    if cache is None:
        cache = TranslationCache()

    translated = []

    for index, word in enumerate(words):
        try:
            assembly = cache.get_or_translate(word)
        except ValueError as error:
            return {
                "translated": translated,
                "error": {
                    "index": index,
                    "word": word,
                    "message": str(error),
                },
            }

        translated.append(assembly)

    return {
        "translated": translated,
        "error": None,
    }
  
