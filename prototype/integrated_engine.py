
"""Integrated reference execution and ARM64 translation validation."""

from prototype.cpu import CPUState
from prototype.program import run_program
from prototype.validation import validate_block


def execute_and_validate(
    instructions,
    start_address=0,
    max_steps=1000,
):
    """Run a guest program and validate its ARM64 translation.

    This does not execute ARM64 machine code natively.
    """
    if not isinstance(instructions, (list, tuple)):
        raise ValueError("Instructions must be a list or tuple")

    if not instructions:
        raise ValueError("Program must not be empty")

    cpu = CPUState()

    steps = run_program(
        cpu,
        instructions,
        start_pc=start_address,
        max_steps=max_steps,
    )

    translation = validate_block(
        instructions,
        start_address=start_address,
    )

    return {
        "status": "passed",
        "execution_steps": steps,
        "final_pc": cpu.pc,
        "registers": {
            f"r{i}": value
            for i, value in enumerate(cpu.gpr)
            if value != 0
        },
        "translation": translation,
        "native_execution": False,
    }
  
