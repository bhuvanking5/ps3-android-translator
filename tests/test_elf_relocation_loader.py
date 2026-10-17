import struct
import unittest

from tools.elf_relocation_loader import ELFLoadError, load_and_relocate


def put16(data, offset, value):
    struct.pack_into(">H", data, offset, value)


def put32(data, offset, value):
    struct.pack_into(">I", data, offset, value)


def put64(data, offset, value):
    struct.pack_into(">Q", data, offset, value)


def make_elf(*, target=0x2000, relocation_type=38, symbol_index=0, addend=4, flags=7):
    data = bytearray(640)
    data[:7] = b"\x7fELF" + bytes((2, 2, 1))
    put16(data, 16, 3)  # ET_DYN
    put16(data, 18, 21)  # EM_PPC64
    put32(data, 20, 1)
    put64(data, 24, 0x1000)  # entry
    put64(data, 32, 64)
    put16(data, 52, 64)
    put16(data, 54, 56)
    put16(data, 56, 2)
    # PT_LOAD covers file bytes and zero-filled BSS through 0x2100.
    put32(data, 64, 1)
    put32(data, 68, flags)
    put64(data, 72, 0)
    put64(data, 80, 0x1000)
    put64(data, 96, len(data))
    put64(data, 104, 0x1100)
    # PT_DYNAMIC at file offset 0x100 / vaddr 0x1100.
    put32(data, 120, 2)
    put64(data, 128, 0x100)
    put64(data, 136, 0x1100)
    put64(data, 152, 9 * 16)
    put64(data, 160, 9 * 16)
    entries = [(5, 0x11A0), (10, 14), (1, 1), (6, 0x1200), (11, 24),
               (7, 0x11C0), (8, 24), (9, 24), (0, 0)]
    for index, (tag, value) in enumerate(entries):
        struct.pack_into(">qQ", data, 0x100 + index * 16, tag, value)
    data[0x1A0:0x1AE] = b"\0libc_stub.so\0"
    struct.pack_into(">QQq", data, 0x1C0, target, (symbol_index << 32) | relocation_type, addend)
    # Symbol 3 is an undefined import.
    struct.pack_into(">IBBHQQ", data, 0x200 + 3 * 24, 1, 0x12, 0, 0, 0, 0)
    return data


class ELFRelocationLoaderTests(unittest.TestCase):
    def test_maps_image_and_applies_supported_relocation(self):
        result = load_and_relocate(make_elf(), load_bias=0x3000)
        self.assertEqual(result["entry"], 0x4000)
        self.assertEqual(result["memory_base"], 0x4000)
        self.assertEqual(result["relocations_applied"], 1)
        offset = 0x5000 - result["memory_base"]
        self.assertEqual(int.from_bytes(result["memory"][offset:offset + 8], "big"), 4)

    def test_requires_resolver_for_undefined_import(self):
        data = make_elf(symbol_index=3)
        with self.assertRaisesRegex(ELFLoadError, "unresolved import"):
            load_and_relocate(data, load_bias=0x3000)

    def test_uses_explicit_resolver_for_import(self):
        result = load_and_relocate(
            make_elf(symbol_index=3), load_bias=0x3000,
            resolve_import=lambda name, symbol: 0x9000 if name == "libc_stub.so" else None,
        )
        offset = 0x5000 - result["memory_base"]
        self.assertEqual(int.from_bytes(result["memory"][offset:offset + 8], "big"), 0x9004)

    def test_rejects_non_writable_relocation_target(self):
        with self.assertRaisesRegex(ELFLoadError, "writable PT_LOAD"):
            load_and_relocate(make_elf(flags=5), load_bias=0x3000)

    def test_rejects_unsupported_relocation_type(self):
        with self.assertRaisesRegex(ELFLoadError, "unsupported PPC64 relocation"):
            load_and_relocate(make_elf(relocation_type=22), load_bias=0x3000)

    def test_rejects_oversized_image_before_mapping(self):
        with self.assertRaisesRegex(ELFLoadError, "memory limit"):
            load_and_relocate(make_elf(), load_bias=0x3000, max_image_bytes=1024)

    def test_rejects_entry_not_in_executable_segment(self):
        with self.assertRaisesRegex(ELFLoadError, "executable"):
            load_and_relocate(make_elf(flags=3), load_bias=0x3000)


if __name__ == "__main__":
    unittest.main()
