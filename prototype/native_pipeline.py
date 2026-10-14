
"""Connect PowerPC translation to ARM64 machine-code encoding."""

from prototype.pipeline import translate_program, translate_word
from prototype.arm64_encoder import encode_program


def translate_word_to_machine_code(word):
    """Translate one PowerPC instruction into ARM64 machine-code bytes."""
    assembly = translate_word(word)
    return encode_program(assembly)


def translate_program_to_machine_code(instructions, start_address=0):
    """Translate a PowerPC program into ARM64 machine-code bytes."""
    assembly = translate_program(
        instructions,
        start_address=start_address,
    )
    return encode_program(assembly)
  
