package org.ps3translator.prototype;

import android.content.ContentResolver;
import android.database.Cursor;
import android.net.Uri;
import android.provider.OpenableColumns;

import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import java.util.Locale;

/**
 * Read-only PS3 PKG header inspector. Does not decrypt, extract, install, or launch.
 */
final class PkgMetadataInspector {
    private static final int HEADER_BYTES = 0xC0;
    private static final long MAX_METADATA_BYTES = 1024L * 1024L;
    private static final long MAX_ITEM_COUNT = 1_000_000L;

    static final class Info {
        final int revision;
        final int type;
        final long metadataOffset;
        final long metadataCount;
        final long metadataSize;
        final long itemCount;
        final long totalSize;
        final long dataOffset;
        final long dataSize;
        final String contentId;
        final List<MetadataEntry> metadataEntries;
        final int metadataTrailingBytes;

        Info(int revision, int type, long metadataOffset, long metadataCount,
             long metadataSize, long itemCount, long totalSize, long dataOffset,
             long dataSize, String contentId, List<MetadataEntry> metadataEntries,
             int metadataTrailingBytes) {
            this.revision = revision;
            this.type = type;
            this.metadataOffset = metadataOffset;
            this.metadataCount = metadataCount;
            this.metadataSize = metadataSize;
            this.itemCount = itemCount;
            this.totalSize = totalSize;
            this.dataOffset = dataOffset;
            this.dataSize = dataSize;
            this.contentId = contentId;
            this.metadataEntries = metadataEntries;
            this.metadataTrailingBytes = metadataTrailingBytes;
        }
    }

    static final class MetadataEntry {
        final long id;
        final long size;
        final byte[] value;

        MetadataEntry(long id, long size, byte[] value) {
            this.id = id;
            this.size = size;
            this.value = value;
        }
    }

    private PkgMetadataInspector() {}

    static long querySize(ContentResolver resolver, Uri uri) {
        Cursor cursor = null;
        try {
            cursor = resolver.query(uri, new String[] {OpenableColumns.SIZE},
                null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int i = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (i >= 0 && !cursor.isNull(i)) return cursor.getLong(i);
            }
        } catch (RuntimeException ignored) {
            // Document provider may omit size metadata.
        } finally {
            if (cursor != null) cursor.close();
        }
        return -1;
    }

    static Info inspect(ContentResolver resolver, Uri uri) throws IOException {
        long fileSize = querySize(resolver, uri);
        if (fileSize < HEADER_BYTES) throw new IOException("PKG is shorter than its 0xC0-byte header.");
        byte[] h = readAt(resolver, uri, 0, HEADER_BYTES, fileSize);
        if (h[0] != 0x7f || h[1] != 'P' || h[2] != 'K' || h[3] != 'G') {
            throw new IOException("PKG magic does not match.");
        }
        int revision = u16(h, 4);
        int type = u16(h, 6);
        long metadataOffset = u32(h, 8);
        long metadataCount = u32(h, 12);
        long metadataSize = u32(h, 16);
        long itemCount = u32(h, 20);
        long totalSize = u64(h, 24);
        long dataOffset = u64(h, 32);
        long dataSize = u64(h, 40);

        if (totalSize < HEADER_BYTES || totalSize != fileSize) {
            throw new IOException("Declared package size does not match the selected file size.");
        }
        if (metadataOffset < HEADER_BYTES || metadataOffset > totalSize
                || metadataSize > totalSize - metadataOffset
                || metadataSize > MAX_METADATA_BYTES || metadataCount > MAX_ITEM_COUNT
                || metadataCount > metadataSize / 8L) {
            throw new IOException("Metadata bounds, record count, or safety limits are invalid.");
        }
        if (itemCount > MAX_ITEM_COUNT || dataOffset > totalSize
                || dataSize > totalSize - dataOffset) {
            throw new IOException("Package data bounds or safety limits are invalid.");
        }
        String contentId = ascii(h, 0x30, 0x30);
        byte[] metadata = readAt(resolver, uri, metadataOffset, (int) metadataSize, fileSize);
        List<MetadataEntry> entries = parseMetadata(metadata, metadataCount);
        int parsedBytes = 0;
        for (MetadataEntry entry : entries) {
            parsedBytes += 8 + (int) entry.size;
        }
        return new Info(revision, type, metadataOffset, metadataCount, metadataSize,
            itemCount, totalSize, dataOffset, dataSize, contentId, entries,
            metadata.length - parsedBytes);
    }

    private static List<MetadataEntry> parseMetadata(byte[] bytes, long declaredCount)
            throws IOException {
        List<MetadataEntry> entries = new ArrayList<>();
        int p = 0;
        for (long i = 0; i < declaredCount; i++) {
            if (p > bytes.length - 8) {
                throw new IOException("Metadata ends before all declared records are present.");
            }
            long id = u32(bytes, p);
            long size = u32(bytes, p + 4);
            p += 8;
            if (size > bytes.length - p) {
                throw new IOException("A metadata record exceeds the declared metadata region.");
            }
            int n = (int) size;
            byte[] value = new byte[n];
            System.arraycopy(bytes, p, value, 0, n);
            entries.add(new MetadataEntry(id, size, value));
            p += n;
        }
        return entries;
    }

    private static byte[] readAt(ContentResolver resolver, Uri uri, long offset,
                                 int count, long fileSize) throws IOException {
        if (offset < 0 || count < 0 || offset > fileSize || count > fileSize - offset) {
            throw new IOException("Read exceeds the selected package.");
        }
        try (InputStream in = resolver.openInputStream(uri)) {
            if (in == null) throw new IOException("Cannot open selected package.");
            long left = offset;
            while (left > 0) {
                long n = in.skip(left);
                if (n > 0) left -= n;
                else {
                    if (in.read() < 0) throw new IOException("Unexpected end of package.");
                    left--;
                }
            }
            byte[] out = new byte[count];
            int pos = 0;
            while (pos < count) {
                int n = in.read(out, pos, count - pos);
                if (n < 0) throw new IOException("Truncated package header.");
                if (n > 0) pos += n;
            }
            return out;
        }
    }

    private static int u16(byte[] b, int p) {
        return ((b[p] & 0xff) << 8) | (b[p + 1] & 0xff);
    }

    private static long u32(byte[] b, int p) {
        return ((b[p] & 0xffL) << 24) | ((b[p + 1] & 0xffL) << 16)
            | ((b[p + 2] & 0xffL) << 8) | (b[p + 3] & 0xffL);
    }

    private static long u64(byte[] b, int p) throws IOException {
        if ((b[p] & 0x80) != 0) throw new IOException("Unsigned 64-bit field exceeds supported range.");
        long value = 0;
        for (int i = 0; i < 8; i++) value = (value << 8) | (b[p + i] & 0xffL);
        return value;
    }

    private static String ascii(byte[] b, int p, int n) {
        int end = p + n;
        while (end > p && (b[end - 1] == 0 || b[end - 1] == ' ')) end--;
        StringBuilder out = new StringBuilder();
        for (int i = p; i < end; i++) {
            int c = b[i] & 0xff;
            out.append(c >= 32 && c <= 126 ? (char)c : '?');
        }
        return out.toString().trim();
    }

    private static String metadataName(long id) {
        switch ((int) id) {
            case 1: return "DRM type";
            case 2: return "Content type";
            case 3: return "Package type/flag";
            case 4: return "Package size";
            case 5: return "Package revision/version";
            case 7: return "QA digest";
            default: return "Unknown metadata";
        }
    }

    private static String hex(byte[] bytes) {
        StringBuilder out = new StringBuilder();
        int limit = Math.min(bytes.length, 32);
        for (int i = 0; i < limit; i++) {
            if (i > 0) out.append(' ');
            out.append(String.format(Locale.ROOT, "%02X", bytes[i] & 0xff));
        }
        if (bytes.length > limit) out.append(" …");
        return out.toString();
    }

    static String format(Info info) {
        String revision = info.revision == 0x8000 ? "finalized/retail (0x8000)"
            : info.revision == 0 ? "debug (0x0000)"
            : String.format(Locale.ROOT, "unknown (0x%04X)", info.revision);
        String type = info.type == 1 ? "PS3 (type 1)"
            : info.type == 2 ? "PSP/Vita (type 2)"
            : String.format(Locale.ROOT, "unknown (type %d)", info.type);
        StringBuilder out = new StringBuilder();
        out.append("PKG header parsed (read-only)\n")
            .append("Revision: ").append(revision).append('\n')
            .append("Package type: ").append(type).append('\n')
            .append("Content ID: ").append(info.contentId.isEmpty() ? "(not present)" : info.contentId).append('\n')
            .append("Item count: ").append(info.itemCount).append('\n')
            .append("Metadata: offset ").append(info.metadataOffset).append(", ")
            .append(info.metadataCount).append(" records declared, ")
            .append(info.metadataSize).append(" bytes\n")
            .append("Metadata records (read-only):\n");
        for (int i = 0; i < info.metadataEntries.size(); i++) {
            MetadataEntry entry = info.metadataEntries.get(i);
            out.append("  ").append(i + 1).append(". ID 0x")
                .append(String.format(Locale.ROOT, "%08X", entry.id))
                .append(" (").append(metadataName(entry.id)).append("), ")
                .append(entry.size).append(" bytes");
            if (entry.value.length > 0) out.append(": ").append(hex(entry.value));
            out.append('\n');
        }
        if (info.metadataTrailingBytes > 0) {
            out.append("Metadata trailer/authentication bytes: ")
                .append(info.metadataTrailingBytes).append(" (left uninterpreted)\n");
        }
        out.append("Data region: offset ").append(info.dataOffset).append(", ")
            .append(info.dataSize).append(" bytes\n")
            .append("Total file size: ").append(info.totalSize).append(" bytes\n\n");

        if (info.type != 1) {
            out.append("Launch assessment: BLOCKED — this is not identified as a PS3 type-1 package.\n");
        } else if (info.revision == 0x8000) {
            out.append("Launch assessment: BLOCKED — finalized/retail PKG detected. ")
                .append("This prototype cannot process protected package contents.\n");
        } else if (info.revision == 0) {
            out.append("Launch assessment: NOT READY — debug-revision PKG detected, but ")
                .append("this prototype still has no PKG item-table parser or bounded payload extractor.\n");
        } else {
            out.append("Launch assessment: NOT READY — package revision is not recognized, ")
                .append("and PKG payload extraction is not implemented.\n");
        }

        out.append("Next safe milestone: parse and validate the package item table, then extract ")
            .append("only supported, unencrypted homebrew files with strict path and size limits. ")
            .append("Do not feed raw PKG bytes to the ELF runner. Metadata was read without decryption; ")
            .append("this report does NOT extract, install, or launch the game. SELF loading, ")
            .append("PS3 firmware services, SPU and RSX remain unsupported.");
        return out.toString();
    }
}
