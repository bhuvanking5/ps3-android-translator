import io
import unittest

from tools.inspect_iso9660 import IsoFormatError, SECTOR_SIZE, parse_iso9660


def both16(value):
    return value.to_bytes(2, "little") + value.to_bytes(2, "big")


def both32(value):
    return value.to_bytes(4, "little") + value.to_bytes(4, "big")


def record(extent, size, identifier, is_dir=False):
    ident = identifier if isinstance(identifier, bytes) else identifier.encode("ascii")
    padding = b"\x00" if len(ident) % 2 == 0 else b""
    length = 33 + len(ident) + len(padding)
    data = bytearray(length)
    data[0] = length
    data[1] = 0
    data[2:10] = both32(extent)
    data[10:18] = both32(size)
    data[25] = 2 if is_dir else 0
    data[28:32] = both16(1)
    data[32] = len(ident)
    data[33:33 + len(ident)] = ident
    return bytes(data)


def make_iso():
    # 24 sectors: PVD at sector 16, root directory at 20, file at 21.
    image = bytearray(24 * SECTOR_SIZE)
    pvd = memoryview(image)[16 * SECTOR_SIZE:17 * SECTOR_SIZE]
    pvd[0] = 1
    pvd[1:6] = b"CD001"
    pvd[6] = 1
    pvd[80:88] = both32(24)
    pvd[128:132] = both16(SECTOR_SIZE)
    root = record(20, SECTOR_SIZE, b"\x00", True)
    pvd[156] = len(root)
    pvd[156:156 + len(root)] = root

    directory = bytearray(SECTOR_SIZE)
    dot = record(20, SECTOR_SIZE, b"\x00", True)
    dotdot = record(20, SECTOR_SIZE, b"\x01", True)
    child = record(21, 4, "EBOOT.BIN;1")
    directory[:len(dot)] = dot
    directory[len(dot):len(dot) + len(dotdot)] = dotdot
    directory[len(dot) + len(dotdot):len(dot) + len(dotdot) + len(child)] = child
    image[20 * SECTOR_SIZE:21 * SECTOR_SIZE] = directory
    image[21 * SECTOR_SIZE:21 * SECTOR_SIZE + 4] = b"TEST"
    return bytes(image)


class Iso9660Tests(unittest.TestCase):
    def test_lists_file_metadata_without_extracting(self):
        image = make_iso()
        parsed = parse_iso9660(io.BytesIO(image), len(image))
        self.assertEqual(parsed.block_size, SECTOR_SIZE)
        self.assertEqual(parsed.volume_blocks, 24)
        self.assertEqual(len(parsed.entries), 1)
        self.assertEqual(parsed.entries[0].path, "EBOOT.BIN")
        self.assertEqual(parsed.entries[0].extent_lba, 21)
        self.assertEqual(parsed.entries[0].size, 4)
        self.assertFalse(parsed.entries[0].is_directory)

    def test_rejects_truncated_image(self):
        image = make_iso()
        with self.assertRaises(IsoFormatError):
            parse_iso9660(io.BytesIO(image[:17 * SECTOR_SIZE]), 17 * SECTOR_SIZE)

    def test_rejects_extent_outside_volume(self):
        image = bytearray(make_iso())
        start = 20 * SECTOR_SIZE
        # Replace the file record extent with an out-of-range LBA while keeping
        # the both-endian encoding internally consistent.
        dir_start = start
        dot_len = image[dir_start]
        dotdot_len = image[dir_start + dot_len]
        file_record = dir_start + dot_len + dotdot_len
        image[file_record + 2:file_record + 10] = both32(99)
        with self.assertRaises(IsoFormatError):
            parse_iso9660(io.BytesIO(image), len(image))

    def test_rejects_entry_limit(self):
        image = bytearray(make_iso())
        directory_start = 20 * SECTOR_SIZE
        first = image[directory_start]
        second = image[directory_start + first]
        third_offset = directory_start + first + second
        third_offset += image[third_offset]  # append after the existing file record
        extra = record(22, 4, "SECOND.BIN;1")
        image[third_offset:third_offset + len(extra)] = extra
        image[22 * SECTOR_SIZE:22 * SECTOR_SIZE + 4] = b"DATA"
        with self.assertRaises(IsoFormatError):
            parse_iso9660(io.BytesIO(image), len(image), max_entries=1)


if __name__ == "__main__":
    unittest.main()
