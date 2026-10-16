
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.cpu import CPUState
from prototype.program import run_program, run_memory_program
from prototype.memory import Memory


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
    )


def conditional_branch(bo, bi, displacement):
    return (
        (16 << 26)
        | (bo << 21)
        | (bi << 16)
        | (((displacement >> 2) & 0x3FFF) << 2)
    )


def branch(displacement):
    return (18 << 26) | ((displacement >> 2) & 0x00FFFFFF) << 2


class ProgramTests(unittest.TestCase):

    def test_memory_reads_big_endian_instruction(self):
        memory = Memory(0x1000, b"\x38\x60\x00\x05")
        self.assertEqual(memory.read_u32_be(0x1000), 0x38600005)

    def test_memory_out_of_bounds(self):
        memory = Memory(0, b"\x00\x00\x00\x00")
        with self.assertRaises(ValueError):
            memory.read_u32_be(4)

    def test_runs_multiple_instructions(self):
        cpu = CPUState()
        program = [
            addi(3, 0, 10),
            addi(4, 3, 5),
        ]

        steps = run_program(cpu, program)

        self.assertEqual(steps, 2)
        self.assertEqual(cpu.gpr[3], 10)
        self.assertEqual(cpu.gpr[4], 15)
        self.assertEqual(cpu.pc, 8)

    def test_branch_skips_instruction(self):
        cpu = CPUState()
        program = [
            branch(8),
            addi(3, 0, 99),
            addi(3, 0, 7),
        ]

        steps = run_program(cpu, program)

        self.assertEqual(steps, 2)
        self.assertEqual(cpu.gpr[3], 7)

    def test_conditional_branch_executes_through_program_runner(self):
        cpu = CPUState()
        cpu.ctr = 2
        program = [
            conditional_branch(16, 0, 8),  # decrement CTR; branch if nonzero
            addi(3, 0, 99),                # must be skipped
            addi(3, 0, 7),
        ]

        steps = run_program(cpu, program)

        self.assertEqual(steps, 2)
        self.assertEqual(cpu.ctr, 1)
        self.assertEqual(cpu.gpr[3], 7)
        self.assertEqual(cpu.pc, 12)

    def test_step_limit_stops_infinite_loop(self):
        cpu = CPUState()
        with self.assertRaises(RuntimeError):
            run_program(cpu, [branch(0)], max_steps=5)


    def test_runs_instructions_directly_from_guest_memory(self):
        cpu = CPUState()
        memory = bytearray(b"\\x00" * 0x20)
        memory[8:16] = addi(3, 0, 7).to_bytes(4, "big") + addi(4, 3, 5).to_bytes(4, "big")
        steps = run_memory_program(cpu, memory, memory_base=0x1000,
                                   start_pc=0x1008, code_size=8)
        self.assertEqual(steps, 2)
        self.assertEqual(cpu.gpr[3], 7)
        self.assertEqual(cpu.gpr[4], 12)
        self.assertEqual(cpu.pc, 0x1010)

    def test_memory_runner_rejects_code_outside_memory(self):
        with self.assertRaisesRegex(ValueError, "bounds"):
            run_memory_program(CPUState(), bytearray(4), memory_base=0x1000,
                               start_pc=0x1000, code_size=8)

    def test_memory_runner_rejects_invalid_step_limit(self):
        with self.assertRaisesRegex(ValueError, "max_steps"):
            run_memory_program(CPUState(), bytearray(4), memory_base=0,
                               start_pc=0, code_size=4, max_steps=0)

    def test_memory_runner_enforces_step_limit(self):
        memory = bytearray(branch(0).to_bytes(4, "big"))
        with self.assertRaisesRegex(RuntimeError, "Maximum"):
            run_memory_program(CPUState(), memory, memory_base=0,
                               start_pc=0, code_size=4, max_steps=2)

if __name__ == "__main__":
    unittest.main()
      
