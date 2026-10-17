import struct
import unittest

from tools.inspect_elf_dynamic import ELFInspectionError, inspect_dynamic_elf


def put16(data, off, value):
    struct.pack_into(">H", data, off, value)


def put32(data, off, value):
    struct.pack_into(">I", data, off, value)


def put64(data, off, value):
    struct.pack_into(">Q", data, off, value)


def make_dynamic_elf():
    data = bytearray(640)
    data[:7] = b"\x7fELF" + bytes((2, 2, 1))
    put16(data, 16, 3)  # ET_DYN
    put16(data, 18, 21)  # EM_PPC64
    put32(data, 20, 1)
    put64(data, 32, 64)
    put16(data, 52, 64)
    put16(data, 54, 56)
    put16(data, 56, 2)

    # PT_LOAD: file offset 0 maps at virtual address 0x1000.
    put32(data, 64, 1)
    put32(data, 68, 5)
    put64(data, 72, 0)
    put64(data, 80, 0x1000)
    put64(data, 96, len(data))
    put64(data, 104, len(data))

    # PT_DYNAMIC at file offset 0x100 / virtual address 0x1100.
    put32(data, 120, 2)
    put64(data, 128, 0x100)
    put64(data, 136, 0x1100)
    put64(data, 152, 9 * 16)
    put64(data, 160, 9 * 16)

    entries = [
        (5, 0x11A0),   # DT_STRTAB
        (10, 14),      # DT_STRSZ
        (1, 1),        # DT_NEEDED, offset 1
        (6, 0x1200),   # DT_SYMTAB
        (11, 24),      # DT_SYMENT
        (7, 0x11C0),   # DT_RELA
        (8, 24),       # DT_RELASZ
        (9, 24),       # DT_RELAENT
        (0, 0),        # DT_NULL
    ]
    for i, (tag, value) in enumerate(entries):
        struct.pack_into(">qQ", data, 0x100 + i * 16, tag, value)
    data[0x180:0x18E] = b"\0libc_stub.so\0"
    # One big-endian Elf64_Rela: target 0x2000, symbol 3, type 22, addend -4.
    struct.pack_into(">QQq", data, 0x1C0, 0x2000, (3 << 32) | 22, -4)
    # Dynamic symbol 3 is an undefined imported function named libc_stub.so.
    struct.pack_into(">IBBHQQ", data, 0x200 + 3 * 24, 1, 0x12, 0, 0, 0, 0)
    return data


class DynamicELFInspectionTests(unittest.TestCase):
    def test_reads_needed_library_and_relocation_metadata(self):
        result = inspect_dynamic_elf(bytes(make_dynamic_elf()))
        self.assertEqual(result["elf_type"], "ET_DYN")
        self.assertTrue(result["has_dynamic_segment"])
        self.assertEqual(result["needed_libraries"], ["libc_stub.so"])
        self.assertEqual(result["relocation_metadata"]["DT_RELA"], 0x11C0)
        self.assertEqual(result["relocation_metadata"]["DT_RELASZ"], 24)
        self.assertEqual(result["relocations"]["DT_RELA"], [
            {"offset": 0x2000, "symbol_index": 3, "type": 22, "addend": -4,
             "symbol": "libc_stub.so", "undefined_import": True}
        ])
        self.assertEqual(result["referenced_symbols"][3]["binding"], 1)
        self.assertTrue(result["referenced_symbols"][3]["undefined_import"])
        self.assertIn("never applied", result["limitations"])

    def test_rejects_truncated_program_header_table(self):
        data = make_dynamic_elf()
        put64(data, 32, len(data) - 10)
        with self.assertRaisesRegex(ELFInspectionError, "truncated"):
            inspect_dynamic_elf(bytes(data))

    def test_rejects_dynamic_table_without_null_terminator(self):
        data = make_dynamic_elf()
        # Replace the final DT_NULL with a non-null tag.
        struct.pack_into(">qQ", data, 0x100 + 8 * 16, 99, 0)
        with self.assertRaisesRegex(ELFInspectionError, "DT_NULL"):
            inspect_dynamic_elf(bytes(data))

    def test_rejects_needed_string_outside_table(self):
        data = make_dynamic_elf()
        struct.pack_into(">qQ", data, 0x100 + 2 * 16, 1, 99)
        with self.assertRaisesRegex(ELFInspectionError, "outside"):
            inspect_dynamic_elf(bytes(data))

    def test_rejects_relocation_table_outside_file_backed_load_segment(self):
        data = make_dynamic_elf()
        struct.pack_into(">qQ", data, 0x100 + 5 * 16, 7, 0x1FFF)
        with self.assertRaisesRegex(ELFInspectionError, "dynamic pointer"):
            inspect_dynamic_elf(bytes(data))

    def test_rejects_wrong_rela_entry_size(self):
        data = make_dynamic_elf()
        struct.pack_into(">qQ", data, 0x100 + 7 * 16, 9, 16)
        with self.assertRaisesRegex(ELFInspectionError, "entry size"):
            inspect_dynamic_elf(bytes(data))

    def test_reports_no_dynamic_segment(self):
        data = make_dynamic_elf()
        put16(data, 56, 1)
        result = inspect_dynamic_elf(bytes(data))
        self.assertFalse(result["has_dynamic_segment"])
        self.assertEqual(result["tags"], [])


if __name__ == "__main__":
    unittest.main()
