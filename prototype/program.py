
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
  


def run_memory_program(cpu, memory, memory_base=0, start_pc=None, code_size=None, max_steps=1000):
    """Execute instructions directly from a mapped guest-memory bytearray.

    The executable range is [start_pc, start_pc + code_size); fetches are
    big-endian and must remain inside both that range and the supplied memory.
    """
    if not isinstance(memory, bytearray):
        raise ValueError("memory must be a bytearray")
    if not isinstance(memory_base, int) or isinstance(memory_base, bool) or memory_base < 0:
        raise ValueError("memory_base must be a non-negative integer")
    if start_pc is None:
        start_pc = memory_base
    if not isinstance(start_pc, int) or isinstance(start_pc, bool) or start_pc < memory_base or start_pc % 4:
        raise ValueError("start_pc must be aligned and inside guest memory")
    if not isinstance(code_size, int) or isinstance(code_size, bool) or code_size < 0 or code_size % 4:
        raise ValueError("code_size must be a non-negative multiple of four")
    if not isinstance(max_steps, int) or isinstance(max_steps, bool) or max_steps <= 0:
        raise ValueError("max_steps must be a positive integer")
    offset = start_pc - memory_base
    if offset + code_size > len(memory):
        raise ValueError("Executable range exceeds guest memory bounds")
    end_pc = start_pc + code_size
    cpu.pc = start_pc
    steps = 0
    while start_pc <= cpu.pc < end_pc:
        if steps >= max_steps:
            raise RuntimeError("Maximum instruction step count reached")
        relative = cpu.pc - memory_base
        if relative < offset or relative + 4 > offset + code_size:
            raise ValueError("Instruction fetch outside executable range")
        word = int.from_bytes(memory[relative:relative + 4], "big")
        cpu.execute(word)
        steps += 1
    return steps
