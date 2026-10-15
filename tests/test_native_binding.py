"""Smoke tests for the optional ps3_native extension.

These tests validate the API boundary only. They never execute supplied machine code.
The main Python suite can run before the optional extension is built; in that case
these tests are skipped and CTest runs them after the extension build.
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
    def test_rejects_empty_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"")

    def test_rejects_unaligned_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"x")

    @unittest.skipIf(
        platform.machine().lower() in ("aarch64", "arm64"),
        "unsupported-architecture error is not expected on ARM64",
    )
    def test_reports_unsupported_architecture(self):
        with self.assertRaisesRegex(RuntimeError, "requires an AArch64 process"):
            ps3_native.execute_u64(b"abcd")


if __name__ == "__main__":
    unittest.main(verbosity=2)
