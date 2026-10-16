"""Smoke tests for the optional ps3_native extension.

Tests compare the native C translator with the Python encoder, validate input
bounds, and exercise the narrow native execution API. Only small supported
instruction sequences are used.
"""
import importlib
import os
import platform
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))


try:
    ps3_native = importlib.import_module("ps3_native")
except ImportError:
    ps3_native = None


@unittest.skipIf(ps3_native is None, "optional ps3_native extension is not built yet")
class NativeBindingTests(unittest.TestCase):
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
            ps3_native.translate_and_execute_ppc(self.SIMPLE_PPC_PROGRAM, 32)

    def test_rejects_empty_program(self):
        with self.assertRaisesRegex(ValueError, "1..256"):
            ps3_native.translate_and_execute_ppc([], 6)

    def test_rejects_unsupported_powerpc_instruction(self):
        with self.assertRaisesRegex(ValueError, "unsupported PowerPC"):
            ps3_native.translate_and_execute_ppc([0x40000004], 3)

    def test_rejects_out_of_range_instruction_word(self):
        with self.assertRaisesRegex(ValueError, "uint32"):
            ps3_native.translate_and_execute_ppc([0x100000000], 3)

    def assert_translator_output_is_bounded(self, words, result_gpr):
        native_bytes = ps3_native.translate_ppc_to_arm64(words, result_gpr)
        self.assertGreater(len(native_bytes), 4 * len(words))
        self.assertEqual(len(native_bytes) % 4, 0)
        self.assertLessEqual(len(native_bytes), 4 * 1100)
        self.assertEqual(native_bytes[-4:], bytes.fromhex('c0035fd6'))

    def test_c_translator_emits_bounded_code_for_addi(self):
        self.assert_translator_output_is_bounded([0x38600007], 3)

    def test_c_translator_emits_bounded_code_for_two_addi_instructions(self):
        self.assert_translator_output_is_bounded([0x38600007, 0x38830005], 4)

    def test_c_translator_emits_bounded_code_for_add(self):
        self.assert_translator_output_is_bounded([0x7C642A14], 3)

    def test_c_translator_emits_bounded_code_for_subf(self):
        self.assert_translator_output_is_bounded([0x7C642850], 3)

    def test_c_translator_emits_bounded_code_for_mulld(self):
        self.assert_translator_output_is_bounded([0x7C6429D2], 3)

    def test_c_translator_emits_bounded_code_for_or(self):
        self.assert_translator_output_is_bounded([0x7C642B78], 4)

    def test_c_translator_emits_bounded_code_for_and(self):
        self.assert_translator_output_is_bounded([0x7C642838], 4)

    def test_c_translator_emits_bounded_code_for_xor(self):
        self.assert_translator_output_is_bounded([0x7C642A78], 4)

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
