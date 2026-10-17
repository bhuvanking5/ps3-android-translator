package org.ps3translator.prototype;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayDeque;
import java.util.HashSet;
import java.util.Locale;
import java.util.Set;

/**
 * Bounded, read-only ISO 9660 metadata walker. It locates the usual PS3 EBOOT.BIN
 * path but never decrypts, extracts to storage, installs, or executes a file.
 * UDF-only images and Joliet-specific names are intentionally not claimed as supported.
 */
final class Iso9660Locator {
    private static final int SECTOR = 2048;
    private static final int MAX_DESCRIPTORS = 32;
    private static final int MAX_DIRECTORY_BYTES = 4 * 1024 * 1024;
    private static final int MAX_ENTRIES = 10000;
    private static final int MAX_DIRECTORIES = 512;
    private static final int MAX_DEPTH = 16;
    private static final int MAX_EBOOT_BYTES = 32 * 1024 * 1024;

    static final class Result {
        final String path;
        final long offset;
        final long size;
        Result(String path, long offset, long size) {
            this.path = path;
            this.offset = offset;
            this.size = size;
        }
    }

    private static final class Directory {
        final String path;
        final long extent;
        final long size;
        final int depth;
        Directory(String path, long extent, long size, int depth) {
            this.path = path;
            this.extent = extent;
            this.size = size;
            this.depth = depth;
        }
    }

    private final ContentResolver resolver;
    private final Uri uri;
    private final long imageSize;
    private long volumeBlocks;

    private Iso9660Locator(ContentResolver resolver, Uri uri, long imageSize) {
        this.resolver = resolver;
        this.uri = uri;
        this.imageSize = imageSize;
    }

    static long querySize(ContentResolver resolver, Uri uri) {
        Cursor cursor = null;
        try {
            cursor = resolver.query(uri, new String[] {OpenableColumns.SIZE},
                null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int index = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (index >= 0 && !cursor.isNull(index)) return cursor.getLong(index);
            }
        } catch (RuntimeException ignored) {
            // Provider does not expose size.
        } finally {
            if (cursor != null) cursor.close();
        }
        return -1L;
    }

    static Result findEboot(ContentResolver resolver, Uri uri) throws IOException {
        long size = querySize(resolver, uri);
        if (size < 0) throw new IOException("The file provider did not report the ISO size.");
        Iso9660Locator parser = new Iso9660Locator(resolver, uri, size);
        return parser.find();
    }

    static byte[] readEbootForRun(ContentResolver resolver, Uri uri, Result result)
            throws IOException {
        long size = querySize(resolver, uri);
        if (size < 0) throw new IOException("The file provider did not report the ISO size.");
        Iso9660Locator parser = new Iso9660Locator(resolver, uri, size);
        if (result.offset < 0 || result.size < 4 || result.size > MAX_EBOOT_BYTES
                || result.offset > size || result.size > size - result.offset) {
            throw new IOException("EBOOT.BIN is outside the experimental 4-byte to 32 MiB limit.");
        }
        return parser.readAt(result.offset, (int) result.size);
    }

    private Result find() throws IOException {
        if (imageSize < 17L * SECTOR) throw new IOException("Image is too small for ISO 9660.");
        byte[] descriptors = readAt(16L * SECTOR, MAX_DESCRIPTORS * SECTOR);
        byte[] pvd = null;
        for (int i = 0; i < MAX_DESCRIPTORS; i++) {
            int at = i * SECTOR;
            if (!asciiEquals(descriptors, at + 1, "CD001") || (descriptors[at + 6] & 0xff) != 1) continue;
            int type = descriptors[at] & 0xff;
            if (type == 1) { pvd = slice(descriptors, at, SECTOR); break; }
            if (type == 255) break;
        }
        if (pvd == null) throw new IOException("No valid ISO 9660 primary volume descriptor found.");
        volumeBlocks = both32(pvd, 80);
        long blockSize = both16(pvd, 128);
        if (volumeBlocks <= 0 || blockSize != SECTOR
                || volumeBlocks > imageSize / SECTOR) {
            throw new IOException("Invalid or unsupported ISO volume geometry.");
        }
        int rootLength = pvd[156] & 0xff;
        if (rootLength < 34 || 156 + rootLength > pvd.length) {
            throw new IOException("Invalid ISO root directory record.");
        }
        byte[] root = slice(pvd, 156, rootLength);
        long rootExtent = both32(root, 2);
        long rootSize = both32(root, 10);
        if ((root[25] & 0x02) == 0) throw new IOException("ISO root record is not a directory.");
        checkExtent(rootExtent, rootSize);

        ArrayDeque<Directory> queue = new ArrayDeque<>();
        queue.add(new Directory("", rootExtent, rootSize, 0));
        Set<String> seen = new HashSet<>();
        int entriesSeen = 0;
        int dirsSeen = 0;
        while (!queue.isEmpty()) {
            Directory dir = queue.removeFirst();
            String key = dir.extent + ":" + dir.size;
            if (!seen.add(key)) continue;
            if (++dirsSeen > MAX_DIRECTORIES) throw new IOException("ISO directory safety limit reached.");
            if (dir.depth > MAX_DEPTH) continue;
            if (dir.size > MAX_DIRECTORY_BYTES) {
                throw new IOException("A directory exceeds the 4 MiB safety limit.");
            }
            byte[] bytes = readAt(dir.extent * SECTOR, (int) dir.size);
            int pos = 0;
            while (pos < bytes.length) {
                int recordLength = bytes[pos] & 0xff;
                if (recordLength == 0) {
                    pos = ((pos / SECTOR) + 1) * SECTOR;
                    continue;
                }
                if (recordLength < 34 || recordLength > bytes.length - pos) {
                    throw new IOException("Malformed ISO directory record.");
                }
                int nameLength = bytes[pos + 32] & 0xff;
                if (nameLength == 0 || 33 + nameLength > recordLength) {
                    throw new IOException("Malformed ISO directory name.");
                }
                long extent = both32(bytes, pos + 2);
                long length = both32(bytes, pos + 10);
                int flags = bytes[pos + 25] & 0xff;
                byte[] rawName = slice(bytes, pos + 33, nameLength);
                pos += recordLength;
                if (rawName.length == 1 && (rawName[0] == 0 || rawName[0] == 1)) continue;
                if (++entriesSeen > MAX_ENTRIES) throw new IOException("ISO entry safety limit reached.");
                String name = new String(rawName, StandardCharsets.US_ASCII);
                int version = name.indexOf(';');
                if (version >= 0) name = name.substring(0, version);
                while (name.endsWith(".")) name = name.substring(0, name.length() - 1);
                name = name.toUpperCase(Locale.ROOT);
                String path = dir.path.isEmpty() ? name : dir.path + "/" + name;
                boolean isDir = (flags & 0x02) != 0;
                checkExtent(extent, length);
                if (!isDir && "PS3_GAME/USRDIR/EBOOT.BIN".equals(path)) {
                    return new Result(path, extent * SECTOR, length);
                }
                if (isDir && dir.depth < MAX_DEPTH && length > 0) {
                    queue.addLast(new Directory(path, extent, length, dir.depth + 1));
                }
            }
        }
        return null;
    }

    private void checkExtent(long extent, long length) throws IOException {
        if (extent < 0 || length < 0 || extent >= volumeBlocks
                || length > (volumeBlocks - extent) * (long) SECTOR) {
            throw new IOException("ISO extent exceeds declared volume.");
        }
        long offset = extent * (long) SECTOR;
        if (offset > imageSize || length > imageSize - offset) {
            throw new IOException("ISO extent exceeds the selected image.");
        }
    }

    private byte[] readAt(long offset, int length) throws IOException {
        if (offset < 0 || length < 0 || offset > imageSize || length > imageSize - offset) {
            throw new IOException("Read would exceed the selected image.");
        }
        try (InputStream input = resolver.openInputStream(uri)) {
            if (input == null) throw new IOException("Cannot open selected image.");
            long remaining = offset;
            while (remaining > 0) {
                long skipped = input.skip(remaining);
                if (skipped > 0) remaining -= skipped;
                else {
                    if (input.read() < 0) throw new IOException("Unexpected end of image.");
                    remaining--;
                }
            }
            ByteArrayOutputStream output = new ByteArrayOutputStream(Math.min(length, 65536));
            byte[] buffer = new byte[Math.min(8192, Math.max(1, length))];
            int left = length;
            while (left > 0) {
                int n = input.read(buffer, 0, Math.min(buffer.length, left));
                if (n < 0) throw new IOException("Truncated image data.");
                if (n == 0) continue;
                output.write(buffer, 0, n);
                left -= n;
            }
            return output.toByteArray();
        }
    }

    private static long both16(byte[] data, int offset) throws IOException {
        if (offset < 0 || offset + 4 > data.length) throw new IOException("Truncated ISO field.");
        int little = (data[offset] & 0xff) | ((data[offset + 1] & 0xff) << 8);
        int big = ((data[offset + 2] & 0xff) << 8) | (data[offset + 3] & 0xff);
        if (little != big) throw new IOException("Inconsistent ISO both-endian field.");
        return little;
    }

    private static long both32(byte[] data, int offset) throws IOException {
        if (offset < 0 || offset + 8 > data.length) throw new IOException("Truncated ISO field.");
        long little = (data[offset] & 0xffL)
            | ((data[offset + 1] & 0xffL) << 8)
            | ((data[offset + 2] & 0xffL) << 16)
            | ((data[offset + 3] & 0xffL) << 24);
        long big = ((data[offset + 4] & 0xffL) << 24)
            | ((data[offset + 5] & 0xffL) << 16)
            | ((data[offset + 6] & 0xffL) << 8)
            | (data[offset + 7] & 0xffL);
        if (little != big || little < 0) throw new IOException("Inconsistent ISO both-endian field.");
        return little;
    }

    private static boolean asciiEquals(byte[] data, int offset, String value) {
        if (offset < 0 || offset + value.length() > data.length) return false;
        for (int i = 0; i < value.length(); i++) {
            if ((data[offset + i] & 0xff) != value.charAt(i)) return false;
        }
        return true;
    }

    private static byte[] slice(byte[] data, int offset, int length) {
        byte[] result = new byte[length];
        System.arraycopy(data, offset, result, 0, length);
        return result;
    }
}
