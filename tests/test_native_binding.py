"""Smoke tests for the optional ps3_native extension.

These tests validate input checking and the bounded translator/runtime API.
The execution test only supplies a tiny supported arithmetic program, never
arbitrary ARM64 bytes.
"""
import importlib
import platform
import unittest

try:
    ps3_native = importlib.import_module("ps3_native")
except ImportError:
    ps3_native = None


@unittest.skipIf(ps3_native is None, "optional ps3_native extension is not built yet")
class NativeBindingTests(unittest.TestCase):
    # addi r3,r0,7; addi r4,r3,5; add r5,r3,r4; subf r6,r3,r5
    SIMPLE_PPC_PROGRAM = [0x38600007, 0x38830005, 0x7CA32214, 0x7CC32850]

    def test_rejects_empty_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"")

    def test_rejects_unaligned_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"x")

    def test_rejects_oversized_code(self):
        with self.assertRaisesRegex(ValueError, "1..4096 bytes"):
            ps3_native.execute_u64(b"\\x00" * 4100)

    @unittest.skipIf(
        platform.machine().lower() in ("aarch64", "arm64"),
        "unsupported-architecture error is not expected on ARM64",
    )
    def test_reports_unsupported_architecture(self):
        with self.assertRaisesRegex(RuntimeError, "requires an AArch64 process"):
            ps3_native.execute_u64(b"abcd")

    def test_rejects_invalid_result_register(self):
        with self.assertRaisesRegex(ValueError, "result_gpr"):
            ps3_native.translate_and_execute_ppc(self.SIMPLE_PPC_PROGRAM, 18)

    def test_rejects_empty_program(self):
        with self.assertRaisesRegex(ValueError, "1..256"):
            ps3_native.translate_and_execute_ppc([], 6)

    def test_rejects_unsupported_powerpc_instruction(self):
        with self.assertRaisesRegex(ValueError, "unsupported PowerPC"):
            ps3_native.translate_and_execute_ppc([0x40000004], 3)

    def test_rejects_out_of_range_instruction_word(self):
        with self.assertRaisesRegex(ValueError, "uint32"):
            ps3_native.translate_and_execute_ppc([0x100000000], 3)

    def test_translate_and_execute_matches_reference_or_rejects_host(self):
        if platform.machine().lower() not in ("aarch64", "arm64"):
            with self.assertRaisesRegex(RuntimeError, "requires an AArch64 process"):
                ps3_native.translate_and_execute_ppc(self.SIMPLE_PPC_PROGRAM, 6)
        else:
            self.assertEqual(
                ps3_native.translate_and_execute_ppc(self.SIMPLE_PPC_PROGRAM, 6),
                12,
            )


if __name__ == "__main__":
    unittest.main(verbosity=2)
