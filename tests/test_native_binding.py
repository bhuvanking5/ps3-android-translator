"""Smoke tests for the optional ps3_native extension.

These tests validate input checking, parity with the Python encoder, and the
bounded native API. The execution test only supplies a tiny supported
arithmetic program, never arbitrary ARM64 bytes.
"""
import importlib
import os
import platform
import sys
import unittest

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..")))

from prototype.native_pipeline import translate_program_to_machine_code

try:
    ps3_native = importlib.import_module("ps3_native")
except ImportError:
    ps3_native = None


@unittest.skipIf(ps3_native is None, "optional ps3_native extension is not built yet")
class NativeBindingTests(unittest.TestCase):
    # addi r3,r0,7; addi r4,r3,5; add r5,r3,r4; subf r6,r3,r5
    SIMPLE_PPC_PROGRAM = [0x38600007, 0x38830005, 0x7CA32214, 0x7CC32850]
    FUNCTION_EPILOGUE_R3 = bytes.fromhex("e00303aac0035fd6")
    FUNCTION_EPILOGUE_R4 = bytes.fromhex("e00304aac0035fd6")

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

    def test_c_translator_matches_python_encoder_for_addi(self):
        words = [0x38600007]
        python_bytes = translate_program_to_machine_code(words)
        native_bytes = ps3_native.translate_ppc_to_arm64(words, 3)
        self.assertEqual(native_bytes, python_bytes + self.FUNCTION_EPILOGUE_R3)

    def test_c_translator_matches_python_encoder_for_two_addi_instructions(self):
        words = [0x38600007, 0x38830005]
        python_bytes = translate_program_to_machine_code(words)
        native_bytes = ps3_native.translate_ppc_to_arm64(words, 4)
        self.assertEqual(native_bytes, python_bytes + self.FUNCTION_EPILOGUE_R4)

    def test_c_translator_matches_python_encoder_for_add(self):
        words = [0x7C642A14]  # add r3, r4, r5
        python_bytes = translate_program_to_machine_code(words)
        native_bytes = ps3_native.translate_ppc_to_arm64(words, 3)
        self.assertEqual(native_bytes, python_bytes + self.FUNCTION_EPILOGUE_R3)

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
