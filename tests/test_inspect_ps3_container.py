import io
import unittest

from tools.inspect_ps3_container import SECTOR_SIZE, inspect_stream


class Ps3ContainerInspectionTests(unittest.TestCase):
    def inspect(self, data):
        return inspect_stream(io.BytesIO(data), len(data))

    def test_recognizes_pkg_magic_without_claiming_validation(self):
        kind, message = self.inspect(b"\x7fPKG" + bytes(128))
        self.assertEqual(kind, "PS3_PKG_SIGNATURE")
        self.assertIn("NOT validated", message)

    def test_recognizes_elf_magic_separately(self):
        kind, _ = self.inspect(b"\x7fELF" + bytes(32))
        self.assertEqual(kind, "ELF_SIGNATURE")

    def test_recognizes_iso9660_descriptor(self):
        data = bytearray(17 * SECTOR_SIZE)
        data[16 * SECTOR_SIZE + 1:16 * SECTOR_SIZE + 6] = b"CD001"
        kind, message = self.inspect(bytes(data))
        self.assertEqual(kind, "ISO9660_SIGNATURE")
        self.assertIn("does not prove", message)

    def test_recognizes_udf_recognition_sequence(self):
        data = bytearray(17 * SECTOR_SIZE)
        data[16 * SECTOR_SIZE + 1:16 * SECTOR_SIZE + 6] = b"NSR03"
        kind, _ = self.inspect(bytes(data))
        self.assertEqual(kind, "UDF_SIGNATURE")

    def test_rejects_tiny_and_unknown_files(self):
        self.assertEqual(self.inspect(b"")[0], "TOO_SMALL")
        self.assertEqual(self.inspect(b"not a game file")[0], "UNKNOWN")


if __name__ == "__main__":
    unittest.main()
