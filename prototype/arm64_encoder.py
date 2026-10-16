
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
    if parts == ["ret"]:
        return 0xD65F03C0
    if len(parts) == 2 and parts[0] == "ret":
        operands = [item.strip() for item in parts[1].split(",")]
        if len(operands) != 1:
            raise ValueError("RET expects one register operand")
        rn = _register(operands[0])
        return 0xD65F0000 | (rn << 5)
    if len(parts) != 2:
        raise ValueError("Invalid assembly instruction")

    mnemonic, operand_text = parts
    operands = [item.strip() for item in operand_text.split(",")]

    # Unconditional branch with a signed byte offset from this instruction.
    if mnemonic == "b" and len(operands) == 1:
        offset_text = operands[0]

        if not offset_text.startswith("#"):
            raise ValueError("Branch offset must begin with '#'")

        try:
            offset = int(offset_text[1:], 0)
        except ValueError as exc:
            raise ValueError("Invalid branch offset") from exc

        if offset % 4 != 0:
            raise ValueError("Branch offset must be 4-byte aligned")

        if not -(1 << 27) <= offset <= (1 << 27) - 4:
            raise ValueError("Branch offset outside AArch64 B range")

        return 0x14000000 | ((offset >> 2) & 0x03FFFFFF)

    # ADD/SUB immediate or ADD register.
    if mnemonic in ("add", "sub") and len(operands) == 3:
        rd = _register(operands[0])
        rn = _register(operands[1])

        if operands[2].startswith("#"):
            imm = _immediate(operands[2])
            base = (
                0x91000000 if mnemonic == "add"
                else 0xD1000000
            )
            return base | (imm << 10) | (rn << 5) | rd

        rm = _register(operands[2])

        base = 0x8B000000 if mnemonic == "add" else 0xCB000000
        return base | (rm << 16) | (rn << 5) | rd

    # ORR register (64-bit).
    if mnemonic == "orr" and len(operands) == 3:
        rd = _register(operands[0])
        rn = _register(operands[1])
        rm = _register(operands[2])
        return 0xAA000000 | (rm << 16) | (rn << 5) | rd

    # MOV immediate or MOV register.
    if mnemonic == "mov" and len(operands) == 2:
        rd = _register(operands[0])

        if operands[1].startswith("#"):
            imm = _immediate(operands[1], maximum=65535)
            return 0xD2800000 | (imm << 5) | rd

        rm = _register(operands[1])
        # MOV Xd, Xm is an alias of ORR Xd, XZR, Xm.
        return 0xAA0003E0 | (rm << 16) | rd

    raise ValueError(f"Unsupported ARM64 instruction: {assembly}")


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
        
