
"""End-to-end validation for translated PowerPC instruction blocks."""

from prototype.branch_block import translate_block


def validate_block(instructions, start_address=0):
    """Translate a block and return a compact validation report.

    This validates translation and output structure only. It does not
    execute the generated ARM64 machine code.
    """
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")

    if not instructions:
        raise ValueError("Instruction block must not be empty")

    machine_code = translate_block(
        instructions,
        start_address=start_address,
    )

    if not machine_code:
        raise ValueError("Translation produced no machine code")

    if len(machine_code) % 4 != 0:
        raise ValueError("ARM64 output is not 4-byte aligned")

    return {
        "status": "passed",
        "guest_instruction_count": len(instructions),
        "arm64_instruction_count": len(machine_code) // 4,
        "machine_code_size": len(machine_code),
        "machine_code_hex": machine_code.hex(),
    }
  
