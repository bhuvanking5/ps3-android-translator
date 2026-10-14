
"""Connect the PowerPC decoder to ARM64 code generation."""

from prototype.ppc_decoder import decode_word


def translate_word(word):
    """Translate one supported PowerPC instruction into ARM64 assembly."""
    ins = decode_word(word)
    op = ins["mnemonic"]

    if op == "addi":
        dst = f'x{ins["rt"]}'
        base_num = ins["ra"]
        imm = ins["immediate"]

        if not -4095 <= imm <= 4095:
            raise ValueError("Immediate outside supported range")

        if base_num == 0:
            if imm >= 0:
                return [f"    mov {dst}, #{imm}"]
            return [
                f"    mov {dst}, #0",
                f"    sub {dst}, {dst}, #{-imm}",
            ]

        base = f"x{base_num}"

        if imm > 0:
            return [f"    add {dst}, {base}, #{imm}"]
        if imm < 0:
            return [f"    sub {dst}, {base}, #{-imm}"]

        return [f"    mov {dst}, {base}"]

    if op == "add":
        if ins["record"]:
            raise ValueError("The add record form is not supported yet")

        dst = f'x{ins["rt"]}'
        left = f'x{ins["ra"]}'
        right = f'x{ins["rb"]}'

        return [f"    add {dst}, {left}, {right}"]

    raise ValueError(f"No code generator for: {op}")


def translate_bytes(data):
    """Translate exactly four big-endian instruction bytes."""
    if not isinstance(data, (bytes, bytearray)) or len(data) != 4:
        raise ValueError("Instruction data must contain exactly 4 bytes")

    return translate_word(int.from_bytes(data, "big"))


def translate_program(instructions, start_address=0):
    """Translate a sequence of supported PowerPC instructions."""
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")

    if (
        not isinstance(start_address, int)
        or isinstance(start_address, bool)
        or start_address < 0
        or start_address % 4 != 0
    ):
        raise ValueError(
            "Start address must be non-negative and 4-byte aligned"
        )

    output = []

    for index, word in enumerate(instructions):
        address = start_address + index * 4

        try:
            output.extend(translate_word(word))
        except ValueError as exc:
            raise ValueError(
                f"Translation failed at 0x{address:08X}: {exc}"
            ) from exc

    return output
        
