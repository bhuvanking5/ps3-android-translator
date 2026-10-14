
import os
import sys
import unittest

sys.path.insert(
    0,
    os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
)

from prototype.cpu import CPUState
from prototype.program import run_program
from prototype.memory import Memory


def addi(rt, ra, immediate):
    return (
        (14 << 26)
        | (rt << 21)
        | (ra << 16)
        | (immediate & 0xFFFF)
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

    def test_step_limit_stops_infinite_loop(self):
        cpu = CPUState()
        with self.assertRaises(RuntimeError):
            run_program(cpu, [branch(0)], max_steps=5)


if __name__ == "__main__":
    unittest.main()
      
