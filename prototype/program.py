
"""Fetch and execute a sequence of PowerPC instructions."""

from prototype.memory import Memory


def run_program(cpu, instructions, start_pc=0, max_steps=1000):
    """Run 32-bit instruction words until execution leaves the program."""
    if not instructions:
        return 0
    if not isinstance(max_steps, int) or isinstance(max_steps, bool):
        raise ValueError("max_steps must be a positive integer")
    if max_steps <= 0:
        raise ValueError("max_steps must be a positive integer")
    if not isinstance(start_pc, int) or start_pc < 0 or start_pc % 4:
        raise ValueError("start_pc must be non-negative and 4-byte aligned")

    raw = bytearray()
    for word in instructions:
        if not isinstance(word, int) or isinstance(word, bool):
            raise ValueError("Instructions must be 32-bit integers")
        if not 0 <= word <= 0xFFFFFFFF:
            raise ValueError("Instruction outside 32-bit range")
        raw.extend(word.to_bytes(4, "big"))

    memory = Memory(start_pc, raw)
    end_pc = start_pc + len(raw)
    cpu.pc = start_pc
    steps = 0

    while start_pc <= cpu.pc < end_pc:
        if steps >= max_steps:
            raise RuntimeError("Maximum instruction step count reached")

        word = memory.read_u32_be(cpu.pc)
        cpu.execute(word)
        steps += 1

    return steps
  
