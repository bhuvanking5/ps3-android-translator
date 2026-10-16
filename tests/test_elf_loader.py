import os
import sys
import struct
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))
from prototype.elf_loader import inspect_ps3_elf, load_elf_segments, ELF_HEADER, PROGRAM_HEADER
from prototype.cpu import CPUState
from prototype.program import run_program


def make_elf(machine=21, elf_class=2, endian=2, truncate=False, payload=b"TEST"):
    ident = bytearray(16)
    ident[:4] = b"\x7fELF"
    ident[4] = elf_class
    ident[5] = endian
    ident[6] = 1
    phoff = ELF_HEADER.size
    header = ELF_HEADER.pack(bytes(ident), 2, machine, 1, 0x10000, phoff, 0, 0,
                             ELF_HEADER.size, PROGRAM_HEADER.size, 1, 0, 0, 0)
    ph = PROGRAM_HEADER.pack(1, 5, ELF_HEADER.size + PROGRAM_HEADER.size,
                             0x10000, 0x10000, len(payload), max(8, len(payload)), 4)
    data = header + ph + payload
    return data[:-2] if truncate else data


class PS3ELFLoaderTests(unittest.TestCase):
    def test_inspects_big_endian_ppc64_elf(self):
        info = inspect_ps3_elf(make_elf())
        self.assertTrue(info["is_executable"])
        self.assertEqual(info["entry_point"], 0x10000)
        self.assertEqual(info["program_header_count"], 1)
        self.assertEqual(info["load_segments"][0]["file_size"], 4)
        self.assertEqual(info["load_segments"][0]["memory_size"], 8)

    def test_rejects_wrong_machine(self):
        with self.assertRaisesRegex(ValueError, "PowerPC64"):
            inspect_ps3_elf(make_elf(machine=62))

    def test_rejects_wrong_endianness(self):
        with self.assertRaisesRegex(ValueError, "big-endian"):
            inspect_ps3_elf(make_elf(endian=1))

    def test_rejects_truncated_segment(self):
        with self.assertRaisesRegex(ValueError, "extends beyond"):
            inspect_ps3_elf(make_elf(truncate=True))

    def test_rejects_non_elf(self):
        with self.assertRaisesRegex(ValueError, "ELF magic"):
            inspect_ps3_elf(b"not an elf file")

    def test_loads_segment_and_zero_fills_bss(self):
        data = make_elf()
        memory = bytearray(0x100)
        entry = load_elf_segments(data, memory, memory_base=0x10000)
        self.assertEqual(entry, 0x10000)
        self.assertEqual(memory[:4], b"TEST")
        self.assertEqual(memory[4:8], b"\x00" * 4)


    def test_loaded_elf_code_executes_in_reference_cpu(self):
        # Synthetic ELF containing two supported PowerPC addi instructions.
        code = struct.pack(">II",
                           (14 << 26) | (3 << 21) | 7,
                           (14 << 26) | (4 << 21) | (3 << 16) | 5)
        data = make_elf(payload=code)
        memory = bytearray(0x100)
        entry = load_elf_segments(data, memory, memory_base=0x10000)
        words = [
            int.from_bytes(memory[i:i + 4], "big")
            for i in range(0, len(code), 4)
        ]
        cpu = CPUState()
        steps = run_program(cpu, words, start_pc=entry)
        self.assertEqual(steps, 2)
        self.assertEqual(cpu.gpr[3], 7)
        self.assertEqual(cpu.gpr[4], 12)
        self.assertEqual(cpu.pc, entry + 8)

    def test_rejects_segment_outside_guest_memory(self):
        with self.assertRaisesRegex(ValueError, "bounds"):
            load_elf_segments(make_elf(), bytearray(4), memory_base=0x10000)

    def test_rejects_segment_below_guest_memory_base(self):
        with self.assertRaisesRegex(ValueError, "below"):
            load_elf_segments(make_elf(), bytearray(0x100), memory_base=0x10001)


if __name__ == "__main__":
    unittest.main()
