"""Connect the PowerPC decoder to ARM64 code generation."""

from prototype.ppc_decoder import decode_word


def _require_supported_registers(ins):
    """Reject GPRs that need the not-yet-implemented r31 spill path."""
    op = ins["mnemonic"]
    registers = [ins["rt"]]

    if op == "addi":
        # PowerPC addi treats RA=0 as a literal zero, not as GPR r0.
        if ins["ra"] != 0:
            registers.append(ins["ra"])
    elif op in ("add", "or"):
        registers.extend([ins["ra"], ins["rb"]])

    if 31 in registers:
        raise ValueError(
            "PowerPC r31 needs a spill/load implementation; "
            "translation to ARM64 is not supported for r31 yet"
        )


def translate_word(word):
    """Translate one supported PowerPC instruction into ARM64 assembly."""
    ins = decode_word(word)
    op = ins["mnemonic"]

    if op == "b":
        if ins["absolute"]:
            raise ValueError("Absolute PowerPC branches are not supported yet")
        if ins["link"]:
            raise ValueError("Linked PowerPC branches (LR update) are not supported yet")
        displacement = ins["displacement"]
        if displacement % 4 != 0:
            raise ValueError("PowerPC branch displacement must be 4-byte aligned")
        return [f"    b #{displacement}"]

    if op in ("addi", "add", "or"):
        _require_supported_registers(ins)

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

    if op == "or":
        if ins["record"]:
            raise ValueError("The or record form is not supported yet")
        return [f'    orr x{ins["ra"]}, x{ins["rt"]}, x{ins["rb"]}']

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
