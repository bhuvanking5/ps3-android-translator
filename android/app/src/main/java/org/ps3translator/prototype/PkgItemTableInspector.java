package org.ps3translator.prototype;

import android.content.ContentResolver;
import android.net.Uri;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;

/**
 * Read-only, bounded inspector for a plaintext/debug PKG item table.
 * It deliberately does not decrypt, extract, install, or launch package data.
 */
final class PkgItemTableInspector {
    private static final int ITEM_BYTES = 0x20;
    private static final int MAX_ITEMS = 10000;
    private static final int MAX_NAME_BYTES = 4096;

    private PkgItemTableInspector() {}

    static String inspect(ContentResolver resolver, Uri uri,
                          PkgMetadataInspector.Info info) throws IOException {
        if (info.type != 1 || info.revision != 0) {
            return "Item-table inspection: skipped. Only PS3 debug-revision PKGs are considered by this plaintext-only inspector.";
        }
        if (info.itemCount <= 0 || info.itemCount > MAX_ITEMS) {
            throw new IOException("Item count is outside the read-only inspector limit (1..10000).");
        }

        PkgMetadataInspector.MetadataEntry tableMeta = null;
        for (PkgMetadataInspector.MetadataEntry entry : info.metadataEntries) {
            if (entry.id == 13) {
                tableMeta = entry;
                break;
            }
        }
        if (tableMeta == null || tableMeta.value.length != 8) {
            return "Item-table inspection: metadata ID 13 is missing or not the expected 8-byte offset/size pair; no payload was read.";
        }

        long tableOffset = u32(tableMeta.value, 0);
        long tableSize = u32(tableMeta.value, 4);
        long expectedSize = info.itemCount * ITEM_BYTES;
        if (tableSize != expectedSize || tableOffset > info.dataSize
                || tableSize > info.dataSize - tableOffset
                || info.dataOffset > info.totalSize
                || tableOffset > info.totalSize - info.dataOffset
                || tableSize > info.totalSize - info.dataOffset - tableOffset) {
            throw new IOException("Item-table offset/size failed bounds validation.");
        }

        byte[] table = readAt(resolver, uri, info.dataOffset + tableOffset, (int) tableSize);
        List<String> names = new ArrayList<>();
        for (int i = 0; i < (int) info.itemCount; i++) {
            int p = i * ITEM_BYTES;
            long nameOffset = u32(table, p);
            long nameLength = u32(table, p + 4);
            long bodyOffset = u64(table, p + 8);
            long bodySize = u64(table, p + 16);
            if (nameLength == 0 || nameLength > MAX_NAME_BYTES
                    || nameOffset > info.dataSize || nameLength > info.dataSize - nameOffset
                    || bodyOffset > info.dataSize || bodySize > info.dataSize - bodyOffset) {
                throw new IOException("Item " + i + " has invalid filename or payload bounds.");
            }
            if (names.size() < 40) {
                byte[] name = readAt(resolver, uri, info.dataOffset + nameOffset, (int) nameLength);
                String decoded = asciiName(name);
                if (decoded == null) {
                    return "Item-table bytes were readable, but filename data is not plain printable ASCII. This may be encrypted or use an unsupported layout; no extraction or launch was attempted.";
                }
                names.add(decoded);
            }
        }

        StringBuilder out = new StringBuilder();
        out.append("Plaintext item-table inspection: validated ")
            .append(info.itemCount).append(" bounded records.\n");
        for (int i = 0; i < names.size(); i++) {
            out.append("  ").append(i + 1).append(". ").append(names.get(i)).append('\n');
        }
        if (info.itemCount > names.size()) {
            out.append("  … ").append(info.itemCount - names.size())
                .append(" more item names not displayed.\n");
        }
        out.append("This is metadata-only: files were not extracted, installed, or launched. Executable/SELF handling and PS3 OS services remain unsupported.");
        return out.toString();
    }

    private static byte[] readAt(ContentResolver resolver, Uri uri, long offset, int count)
            throws IOException {
        try (InputStream in = resolver.openInputStream(uri)) {
            if (in == null) throw new IOException("Cannot reopen selected PKG.");
            long left = offset;
            while (left > 0) {
                long skipped = in.skip(left);
                if (skipped > 0) left -= skipped;
                else {
                    if (in.read() < 0) throw new IOException("Unexpected end of PKG while seeking.");
                    left--;
                }
            }
            byte[] out = new byte[count];
            int pos = 0;
            while (pos < count) {
                int n = in.read(out, pos, count - pos);
                if (n < 0) throw new IOException("Truncated PKG item table or filename.");
                if (n > 0) pos += n;
            }
            return out;
        }
    }

    private static String asciiName(byte[] bytes) {
        StringBuilder out = new StringBuilder(bytes.length);
        for (byte b : bytes) {
            int c = b & 0xff;
            if (c < 32 || c > 126) return null;
            out.append((char) c);
        }
        String value = out.toString();
        if (value.startsWith("/") || value.indexOf('\\') >= 0) return null;
        for (String part : value.split("/")) {
            if (part.equals("..") || part.equals(".")) return null;
        }
        return value;
    }

    private static long u32(byte[] b, int p) {
        return ((b[p] & 0xffL) << 24) | ((b[p + 1] & 0xffL) << 16)
            | ((b[p + 2] & 0xffL) << 8) | (b[p + 3] & 0xffL);
    }

    private static long u64(byte[] b, int p) throws IOException {
        if ((b[p] & 0x80) != 0) throw new IOException("Unsigned item size exceeds supported range.");
        long value = 0;
        for (int i = 0; i < 8; i++) value = (value << 8) | (b[p + i] & 0xffL);
        return value;
    }
}
