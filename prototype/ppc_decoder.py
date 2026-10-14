
"""Decode a small subset of 32-bit PowerPC instructions."""

def decode_word(word):
    """Decode one PowerPC instruction represented as a 32-bit integer."""
    if not isinstance(word, int) or isinstance(word, bool):
        raise ValueError("Instruction must be a 32-bit integer")

    if not 0 <= word <= 0xFFFFFFFF:
        raise ValueError("Instruction must be between 0 and 0xFFFFFFFF")

    opcode = (word >> 26) & 0x3F

    # addi RT, RA, SI — opcode 14, signed 16-bit immediate
    if opcode == 14:
        rt = (word >> 21) & 0x1F
        ra = (word >> 16) & 0x1F
        immediate = word & 0xFFFF

        if immediate & 0x8000:
            immediate -= 0x10000

        return {
            "mnemonic": "addi",
            "rt": rt,
            "ra": ra,
            "immediate": immediate,
        }

    # add RT, RA, RB — extended opcode 266
    if opcode == 31:
        extended_opcode = (word >> 1) & 0x3FF

        if extended_opcode == 266:
            return {
                "mnemonic": "add",
                "rt": (word >> 21) & 0x1F,
                "ra": (word >> 16) & 0x1F,
                "rb": (word >> 11) & 0x1F,
                "record": bool(word & 1),
            }

    raise ValueError(f"Unsupported PowerPC instruction: 0x{word:08X}")


def decode_bytes(data):
    """Decode exactly four instruction bytes in PowerPC big-endian order."""
    if not isinstance(data, (bytes, bytearray)):
        raise ValueError("Instruction data must be bytes")

    if len(data) != 4:
        raise 
    # Unconditional branch: b, ba, bl, or bla
    if opcode == 18:
        li = (word >> 2) & 0x00FFFFFF

        # Sign-extend the 24-bit branch displacement
        if li & 0x00800000:
            li -= 0x01000000

        return {
            "mnemonic": "b",
            "displacement": li << 2,
            "absolute": bool(word & 0x2),
            "link": bool(word & 0x1),
    }
        

    return decode_word(int.from_bytes(data, "big"))
  
