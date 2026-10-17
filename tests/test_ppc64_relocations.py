import unittest

from tools.ppc64_relocations import (
    R_PPC64_ADDR32,
    R_PPC64_ADDR64,
    R_PPC64_REL64,
    RelocationError,
    apply_rela,
    apply_rela_records,
)


class PPC64RelocationTests(unittest.TestCase):
    def test_addr32_writes_big_endian_s_plus_a(self):
        memory = bytearray(8)
        apply_rela(memory, 2, R_PPC64_ADDR32, 0x1000, 0x24, 0)
        self.assertEqual(memory, b"\x00\x00\x00\x00\x10\x24\x00\x00")

    def test_addr64_writes_big_endian_s_plus_a(self):
        memory = bytearray(8)
        apply_rela(memory, 0, R_PPC64_ADDR64, 0x123400000000, 0x55, 0)
        self.assertEqual(memory, bytes.fromhex("0000123400000055"))

    def test_rel64_writes_signed_s_plus_a_minus_p(self):
        memory = bytearray(8)
        apply_rela(memory, 0, R_PPC64_REL64, 0x1000, -8, 0x1010)
        self.assertEqual(int.from_bytes(memory, "big", signed=True), -0x18)

    def test_rejects_unsupported_type_without_mutating_memory(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "unsupported"):
            apply_rela(memory, 0, 999, 1, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_out_of_bounds_target_without_mutation(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "outside"):
            apply_rela(memory, 6, R_PPC64_ADDR32, 1, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_unsigned_overflow_without_mutation(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "out of range"):
            apply_rela(memory, 0, R_PPC64_ADDR32, 1 << 32, 0, 0)
        self.assertEqual(memory, b"abcdefgh")

    def test_rejects_negative_absolute_result(self):
        memory = bytearray(b"abcdefgh")
        with self.assertRaisesRegex(RelocationError, "out of range"):
            apply_rela(memory, 0, R_PPC64_ADDR64, 0, -1, 0)
        self.assertEqual(memory, b"abcdefgh")




    def test_applies_et_dyn_bias_to_target_and_defined_symbol(self):
        memory = bytearray(16)
        symbols = {1: {"name": "local", "value": 0x1010, "section_index": 1}}
        count = apply_rela_records(
            memory, 0x3000,
            [{"offset": 0x2000, "type": R_PPC64_ADDR64, "addend": 4, "symbol_index": 1}],
            symbols, elf_type="ET_DYN", load_bias=0x1000,
        )
        self.assertEqual(count, 1)
        self.assertEqual(int.from_bytes(memory[:8], "big"), 0x2014)

    def test_requires_explicit_import_resolution(self):
        memory = bytearray(8)
        symbols = {2: {"name": "sce_stub", "section_index": 0, "undefined_import": True}}
        with self.assertRaisesRegex(RelocationError, "unresolved import"):
            apply_rela_records(
                memory, 0x1000,
                [{"offset": 0x1000, "type": R_PPC64_ADDR64, "addend": 0, "symbol_index": 2}],
                symbols, elf_type="ET_EXEC",
            )
        self.assertEqual(memory, bytearray(8))

    def test_resolves_import_only_through_explicit_callback(self):
        memory = bytearray(8)
        symbols = {2: {"name": "sce_stub", "section_index": 0, "undefined_import": True}}
        count = apply_rela_records(
            memory, 0x1000,
            [{"offset": 0x1000, "type": R_PPC64_ADDR64, "addend": 8, "symbol_index": 2}],
            symbols, elf_type="ET_EXEC",
            resolve_import=lambda name, _symbol: 0x9000 if name == "sce_stub" else None,
        )
        self.assertEqual(count, 1)
        self.assertEqual(int.from_bytes(memory, "big"), 0x9008)

    def test_entire_relocation_list_is_atomic(self):
        memory = bytearray(b"abcdefghijklmnop")
        original = bytearray(memory)
        relocations = [
            {"offset": 0x1000, "type": R_PPC64_ADDR64, "addend": 1, "symbol_index": 0},
            {"offset": 0x100c, "type": R_PPC64_ADDR64, "addend": 2, "symbol_index": 0},
        ]
        with self.assertRaisesRegex(RelocationError, "outside"):
            apply_rela_records(memory, 0x1000, relocations, {}, elf_type="ET_EXEC")
        self.assertEqual(memory, original)

    def test_rejects_load_bias_for_et_exec(self):
        with self.assertRaisesRegex(RelocationError, "zero load bias"):
            apply_rela_records(bytearray(8), 0, [], {}, elf_type="ET_EXEC", load_bias=1)

if __name__ == "__main__":
    unittest.main()
