
"""Encode a small, supported subset of ARM64 instructions."""

import re


REGISTER_RE = re.compile(r"x([0-9]|[12][0-9]|30)$")


def _register(text):
    """Parse an ARM64 general-purpose register x0-x30."""
    match = REGISTER_RE.fullmatch(text.strip())
    if not match:
        raise ValueError(f"Unsupported ARM64 register: {text}")

    return int(match.group(1))


def _immediate(text, maximum=4095):
    """Parse a non-negative immediate within the supported range."""
    if not text.startswith("#"):
        raise ValueError("Expected an immediate beginning with '#'")

    try:
        value = int(text[1:], 0)
    except ValueError as exc:
        raise ValueError("Invalid immediate value") from exc

    if not 0 <= value <= maximum:
        raise ValueError("Immediate outside supported range")

    return value


def encode_instruction(assembly):
    """Encode one supported ARM64 instruction as a 32-bit integer."""
    if not isinstance(assembly, str):
        raise ValueError("Assembly instruction must be a string")

    parts = assembly.strip().lower().split(None, 1)
    if len(parts) != 2:
        raise ValueError("Invalid assembly instruction")

    mnemonic, operand_text = parts
    operands = [item.strip() for item in operand_text.split(",")]

    if mnemonic in ("add", "sub") and len(operands) == 3:
        rd = _register(operands[0])
        rn = _register(operands[1])

        if operands[2].startswith("#"):
            imm = _immediate(operands[2])
            base = 0x91000000 if mnemonic == "add" else 0xD1000000
            return base | (imm << 10) | (rn << 5) | rd

        rm = _register(operands[2])

        if mnemonic == "add":
            return 0x8B000000 | (rm << 16) | (rn << 5) | rd

        raise ValueError("Register-form SUB is not supported yet")

    if mnemonic == "mov" and len(operands) == 2:
        rd = _register(operands[0])

        if operands[1].startswith("#"):
            imm = _immediate(operands[1], maximum=65535)
            return 0xD2800000 | (imm << 5) | rd

        rm = _register(operands[1])
        # MOV Xd, Xm is an alias of ORR Xd, XZR, Xm.
        return 0xAA0003E0 | (rm << 16) | rd

    raise ValueError(
        f"Unsupported ARM64 instruction: {assembly}"
    )


def encode_bytes(assembly):
    """Return the instruction as four little-endian ARM64 bytes."""
    word = encode_instruction(assembly)
    return word.to_bytes(4, byteorder="little")


def encode_program(instructions):
    """Encode a sequence of assembly lines into machine-code bytes."""
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")

    output = bytearray()

    for index, instruction in enumerate(instructions):
        try:
            output.extend(encode_bytes(instruction))
        except ValueError as exc:
            raise ValueError(
                f"Encoding failed at instruction {index}: {exc}"
            ) from exc

    return bytes(output)
          
