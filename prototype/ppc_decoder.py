"""Decode a small subset of 32-bit PowerPC instructions."""


def decode_word(word):
    """Decode one PowerPC instruction represented as a 32-bit integer."""
    if not isinstance(word, int) or isinstance(word, bool):
        raise ValueError("Instruction must be a 32-bit integer")

    if not 0 <= word <= 0xFFFFFFFF:
        raise ValueError("Instruction must be between 0 and 0xFFFFFFFF")

    opcode = (word >> 26) & 0x3F

    # addi RT, RA, SI — opcode 14
    if opcode == 14:
        rt = (word >> 21) & 0x1F
        ra = (word >> 16) & 0x1F
        immediate = word & 0xFFFF
        if immediate & 0x8000:
            immediate -= 0x10000
        return {"mnemonic": "addi", "rt": rt, "ra": ra, "immediate": immediate}

    # Logical immediates: RT is source, RA is destination.
    if opcode in (24, 26):
        return {
            "mnemonic": "ori" if opcode == 24 else "xori",
            "rt": (word >> 21) & 0x1F,
            "ra": (word >> 16) & 0x1F,
            "immediate": word & 0xFFFF,
        }

    # Compare immediate signed word: cmpwi — opcode 11.
    if opcode == 11:
        immediate = word & 0xFFFF
        if immediate & 0x8000:
            immediate -= 0x10000
        return {
            "mnemonic": "cmpwi",
            "bf": (word >> 23) & 0x7,
            "ra": (word >> 16) & 0x1F,
            "immediate": immediate,
            "is_64bit": bool((word >> 21) & 1),
        }

    # Conditional branch: bc / bca / bcl / bcla — opcode 16.
    if opcode == 16:
        bd = (word >> 2) & 0x3FFF
        if bd & 0x2000:
            bd -= 0x4000
        return {
            "mnemonic": "bc",
            "bo": (word >> 21) & 0x1F,
            "bi": (word >> 16) & 0x1F,
            "displacement": bd << 2,
            "absolute": bool(word & 0x2),
            "link": bool(word & 0x1),
        }

    # Unconditional branch: b, ba, bl, or bla
    if opcode == 18:
        li = (word >> 2) & 0x00FFFFFF
        if li & 0x00800000:
            li -= 0x01000000
        return {
            "mnemonic": "b",
            "displacement": li << 2,
            "absolute": bool(word & 0x2),
            "link": bool(word & 0x1),
        }

    # Extended opcode 31 arithmetic and logical instructions.
    if opcode == 31:
        extended_opcode = (word >> 1) & 0x3FF
        common = {
            "rt": (word >> 21) & 0x1F,
            "ra": (word >> 16) & 0x1F,
            "rb": (word >> 11) & 0x1F,
            "record": bool(word & 1),
        }
        mnemonics = {
            233: "mulld",
            266: "add",
            40: "subf",
            444: "or",
            28: "and",
            316: "xor",
        }
        if extended_opcode in mnemonics:
            return {"mnemonic": mnemonics[extended_opcode], **common}

    raise ValueError(f"Unsupported PowerPC instruction: 0x{word:08X}")


def decode_bytes(data):
    """Decode exactly four instruction bytes in PowerPC big-endian order."""
    if not isinstance(data, (bytes, bytearray)):
        raise ValueError("Instruction data must be bytes")
    if len(data) != 4:
        raise ValueError("An instruction must contain exactly 4 bytes")
    return decode_word(int.from_bytes(data, "big"))
