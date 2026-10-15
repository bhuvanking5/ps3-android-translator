"""Smoke tests for the optional ps3_native extension.

These tests validate the API boundary only. They never execute supplied machine code.
"""
import platform
import unittest

try:
    import ps3_native
except ImportError as exc:
    raise SystemExit(f"ps3_native extension was not built or found: {exc}")


class NativeBindingTests(unittest.TestCase):
    def test_rejects_empty_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"")

    def test_rejects_unaligned_code(self):
        with self.assertRaises(ValueError):
            ps3_native.execute_u64(b"\\x00")

    @unittest.skipIf(
        platform.machine().lower() in ("aarch64", "arm64"),
        "unsupported-architecture error is not expected on ARM64",
    )
    def test_reports_unsupported_architecture(self):
        with self.assertRaisesRegex(RuntimeError, "requires an AArch64 process"):
            ps3_native.execute_u64(b"\\x1f\\x20\\x03\\xd5")


if __name__ == "__main__":
    unittest.main(verbosity=2)
