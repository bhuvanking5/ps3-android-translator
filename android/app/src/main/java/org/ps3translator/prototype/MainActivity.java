package org.ps3translator.prototype;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.graphics.Typeface;
import android.graphics.drawable.GradientDrawable;
import android.net.Uri;
import android.database.Cursor;
import android.provider.OpenableColumns;
import android.view.Gravity;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.LinearLayout;
import android.widget.ScrollView;
import android.widget.SeekBar;
import android.widget.Spinner;
import android.widget.ArrayAdapter;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private static final int PICK_GAME = 41;
    private static final int PICK_FIRMWARE = 42;
    private static final String PREFS = "prototype_settings";
    private LinearLayout layout;
    private TextView selectedGame;
    private TextView selectedFirmware;
    private TextView resolutionLabel;
    private Spinner frameCap;
    private SeekBar resolution;
    private CheckBox lowMemory;
    private CheckBox simpleEffects;
    private SharedPreferences prefs;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        prefs = getSharedPreferences(PREFS, MODE_PRIVATE);

        ScrollView scroll = new ScrollView(this);
        layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setPadding(dp(18), dp(22), dp(18), dp(30));
        layout.setBackgroundColor(Color.rgb(10, 15, 28));
        scroll.setFillViewport(true);
        scroll.addView(layout);

        LinearLayout hero = new LinearLayout(this);
        hero.setOrientation(LinearLayout.VERTICAL);
        hero.setPadding(dp(20), dp(22), dp(20), dp(20));
        hero.setBackground(roundedBackground(
            new int[] {Color.rgb(23, 48, 82), Color.rgb(17, 27, 48)}, dp(24)));
        TextView eyebrow = text("ANDROID RESEARCH BUILD", 11, Color.rgb(94, 234, 212));
        eyebrow.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        eyebrow.setLetterSpacing(0.08f);
        hero.addView(eyebrow);

        TextView title = text("PS3 TRANSLATOR", 27, Color.WHITE);
        title.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        title.setPadding(0, dp(8), 0, dp(3));
        hero.addView(title);

        TextView subtitle = text("POWERPC  →  ARM64", 14, Color.rgb(147, 197, 253));
        subtitle.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        hero.addView(subtitle);

        TextView status = text(nativeStatus(), 14, Color.rgb(226, 232, 240));
        status.setPadding(dp(12), dp(12), dp(12), dp(12));
        status.setBackground(roundedBackground(
            new int[] {Color.rgb(15, 23, 42), Color.rgb(15, 23, 42)}, dp(14)));
        status.setLineSpacing(dp(3), 1.0f);
        LinearLayout.LayoutParams statusParams = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        statusParams.topMargin = dp(18);
        hero.addView(status, statusParams);
        layout.addView(hero);

        heading("PS3 firmware");
        Button addFirmware = new Button(this);
        styleButton(addFirmware);
        addFirmware.setText("Choose firmware file (.PUP)");
        addFirmware.setOnClickListener(v -> chooseFirmware());
        layout.addView(addFirmware);
        String savedFirmware = prefs.getString("selected_firmware_uri", null);
        selectedFirmware = text(savedFirmware == null
            ? "No firmware selected. Use an official PS3UPDAT.PUP file you obtained lawfully."
            : "Previously selected firmware URI: " + savedFirmware
                + "\nSaved for reference only; firmware is not installed or loaded by this prototype.",
            14, Color.LTGRAY);
        selectedFirmware.setPadding(0, 8, 0, 8);
        layout.addView(selectedFirmware);
        TextView firmwareNote = text(
            "Firmware selection is only a UI milestone. This prototype does not install, decrypt, or boot PS3 firmware.",
            13, Color.rgb(255, 205, 130));
        firmwareNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(firmwareNote);

        heading("Games");
        Button addGame = new Button(this);
        styleButton(addGame);
        addGame.setText("Choose game file");
        addGame.setOnClickListener(v -> chooseGame());
        layout.addView(addGame);
        String savedGame = prefs.getString("selected_game_uri", null);
        selectedGame = text(savedGame == null
            ? "No file selected. Choosing a file does not launch it."
            : "Previously selected file URI:\n" + savedGame
                + "\nSaved for reference only; PS3 game boot is not implemented.", 14, Color.LTGRAY);
        selectedGame.setPadding(0, 8, 0, 8);
        layout.addView(selectedGame);

        Button launch = new Button(this);
        styleButton(launch);
        launch.setText("Try launching supported ELF (experimental)");
        launch.setOnClickListener(v -> inspectLaunchSelectedGame());
        layout.addView(launch);
        TextView launchNote = text(
            "Experimental: inspects PKG headers and attempts a bounded interpreter run only for small, unencrypted ELF64 PowerPC64 files. PKG installation/decryption, SELF games, firmware, OS services and RSX graphics are not implemented.",
            13, Color.rgb(255, 205, 130));
        launchNote.setPadding(0, 4, 0, 8);
        launchNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(launchNote);

        Button elfPipelineTest = new Button(this);
        styleButton(elfPipelineTest);
        elfPipelineTest.setText("Run built-in ELF execution test");
        elfPipelineTest.setOnClickListener(v -> {
            selectedGame.setText("Running built-in ELF loader + PowerPC interpreter test…");
            new Thread(() -> {
                final String result = nativeRunBuiltinElfTest();
                runOnUiThread(() -> selectedGame.setText(
                    "Built-in ELF pipeline test:\n" + result
                    + "\nExpected arithmetic result: r3=7, r4=5, r5=12 (0xc)."));
            }, "ps3-builtin-elf-test").start();
        });
        layout.addView(elfPipelineTest);
        TextView elfPipelineNote = text(
            "Runs a tiny executable bundled in the app through the actual native ELF loader and bounded PowerPC interpreter. It checks the launch pipeline only; it is not a PS3 game.",
            13, Color.rgb(255, 205, 130));
        elfPipelineNote.setPadding(0, 4, 0, 8);
        elfPipelineNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(elfPipelineNote);

        Button controller = new Button(this);
        styleButton(controller);
        controller.setText("Open virtual controller");
        controller.setOnClickListener(v ->
            startActivity(new Intent(this, VirtualControllerActivity.class)));
        layout.addView(controller);
        TextView controllerNote = text(
            "Touch D-pad, sticks and action buttons. This layout test is not connected to game emulation yet.",
            13, Color.rgb(255, 205, 130));
        controllerNote.setPadding(0, 8, 0, 8);
        controllerNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(controllerNote);

        Button demoGame = new Button(this);
        styleButton(demoGame);
        demoGame.setText("Run built-in 2D performance test");
        demoGame.setOnClickListener(v ->
            startActivity(new Intent(this, TwoDTestActivity.class)));
        layout.addView(demoGame);
        TextView demoNote = text(
            "This is a small Android test game for checking drawing, touch input and FPS. It is NOT a PS3 game and does not test PS3 compatibility.",
            13, Color.rgb(255, 205, 130));
        demoNote.setPadding(0, 8, 0, 8);
        demoNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(demoNote);

        Button demo3d = new Button(this);
        styleButton(demo3d);
        demo3d.setText("Run built-in 3D OpenGL test");
        demo3d.setOnClickListener(v ->
            startActivity(new Intent(this, ThreeDTestActivity.class)));
        layout.addView(demo3d);
        TextView demo3dNote = text(
            "Rotating 3D cube for checking Android OpenGL ES rendering. This is not PS3 graphics emulation.",
            13, Color.rgb(255, 205, 130));
        demo3dNote.setPadding(0, 8, 0, 8);
        demo3dNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(demo3dNote);

        Button graphics = new Button(this);
        styleButton(graphics);
        graphics.setText("Run graphics translation scaffold");
        graphics.setOnClickListener(v ->
            startActivity(new Intent(this, GraphicsTranslationTestActivity.class)));
        layout.addView(graphics);
        TextView graphicsNote = text(
            "Experimental internal CLEAR/DRAW command mapping to OpenGL ES 2.0. This is the first graphics-pipeline scaffold, not a PS3 RSX decoder.",
            13, Color.rgb(255, 205, 130));
        graphicsNote.setPadding(0, 8, 0, 8);
        graphicsNote.setLineSpacing(dp(3), 1.0f);
        layout.addView(graphicsNote);

        heading("Performance settings");
        TextView preset = text("Default preset: Low-end device (conservative settings)", 14, Color.rgb(130, 230, 160));
        layout.addView(preset);

        resolutionLabel = text("", 15, Color.WHITE);
        layout.addView(resolutionLabel);
        resolution = new SeekBar(this);
        resolution.setMax(2);
        resolution.setProgress(prefs.getInt("resolution", 0));
        resolution.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            public void onProgressChanged(SeekBar bar, int value, boolean fromUser) {
                String[] values = {"25% (very low)", "50% (low)", "75% (balanced)"};
                resolutionLabel.setText("Internal resolution target: " + values[value]);
                if (fromUser) prefs.edit().putInt("resolution", value).apply();
            }
            public void onStartTrackingTouch(SeekBar bar) {}
            public void onStopTrackingTouch(SeekBar bar) {}
        });
        layout.addView(resolution);

        TextView capLabel = text("Frame-rate limit (UI preference only)", 15, Color.WHITE);
        capLabel.setPadding(0, 12, 0, 4);
        layout.addView(capLabel);
        frameCap = new Spinner(this);
        String[] caps = {"30 FPS (recommended)", "60 FPS (more demanding)"};
        frameCap.setAdapter(new ArrayAdapter<String>(this, android.R.layout.simple_spinner_dropdown_item, caps));
        frameCap.setSelection(prefs.getInt("frame_cap", 0));
        frameCap.setOnItemSelectedListener(new android.widget.AdapterView.OnItemSelectedListener() {
            public void onItemSelected(android.widget.AdapterView<?> parent, android.view.View view, int position, long id) {
                prefs.edit().putInt("frame_cap", position).apply();
            }
            public void onNothingSelected(android.widget.AdapterView<?> parent) {}
        });
        layout.addView(frameCap);

        lowMemory = new CheckBox(this);
        lowMemory.setText("Prefer lower memory use");
        lowMemory.setTextColor(Color.WHITE);
        lowMemory.setChecked(prefs.getBoolean("low_memory", true));
        lowMemory.setOnCheckedChangeListener((button, checked) -> prefs.edit().putBoolean("low_memory", checked).apply());
        layout.addView(lowMemory);

        simpleEffects = new CheckBox(this);
        simpleEffects.setText("Prefer simpler graphics effects");
        simpleEffects.setTextColor(Color.WHITE);
        simpleEffects.setChecked(prefs.getBoolean("simple_effects", true));
        simpleEffects.setOnCheckedChangeListener((button, checked) -> prefs.edit().putBoolean("simple_effects", checked).apply());
        layout.addView(simpleEffects);

        TextView caveat = text("This is not yet a PS3 emulator. PKG support currently validates bounded header metadata only; it does not extract, decrypt, install or launch PKG contents. The experimental ELF path loads only a small unencrypted ELF into a bounded guest-memory model and interprets a tiny PowerPC subset. SELF handling, full relocations/imports, stack/process setup, firmware, SPU and RSX are not implemented. No real PS3 game has booted.", 13, Color.LTGRAY);
        caveat.setPadding(0, 18, 0, 0);
        caveat.setLineSpacing(dp(3), 1.0f);
        caveat.setBackground(roundedBackground(new int[] {Color.rgb(23, 31, 48), Color.rgb(23, 31, 48)}, dp(14)));
        caveat.setPadding(dp(14), dp(14), dp(14), dp(14));
        layout.addView(caveat);
        setContentView(scroll);
    }

    private int dp(float value) {
        return (int) (value * getResources().getDisplayMetrics().density + 0.5f);
    }

    private android.graphics.drawable.GradientDrawable roundedBackground(int[] colors, int radius) {
        android.graphics.drawable.GradientDrawable drawable =
            new android.graphics.drawable.GradientDrawable(
                android.graphics.drawable.GradientDrawable.Orientation.TL_BR, colors);
        drawable.setCornerRadius(radius);
        return drawable;
    }

    private void styleButton(Button button) {
        button.setAllCaps(false);
        button.setTextColor(Color.rgb(8, 20, 37));
        button.setTextSize(15);
        button.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        button.setPadding(dp(16), dp(12), dp(16), dp(12));
        button.setMinHeight(dp(52));
        button.setBackground(roundedBackground(
            new int[] {Color.rgb(94, 234, 212), Color.rgb(56, 189, 248)}, dp(16)));
        button.setElevation(dp(2));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.WRAP_CONTENT);
        params.topMargin = dp(5);
        params.bottomMargin = dp(5);
        button.setLayoutParams(params);
    }

    private TextView text(String value, int size, int color) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        view.setTypeface(Typeface.create("sans-serif", Typeface.NORMAL));
        view.setLineSpacing(dp(2), 1.0f);
        return view;
    }

    private void heading(String value) {
        TextView view = text(value.toUpperCase(java.util.Locale.ROOT), 14,
            Color.rgb(125, 211, 252));
        view.setTypeface(Typeface.DEFAULT, Typeface.BOLD);
        view.setLetterSpacing(0.07f);
        view.setPadding(0, dp(24), 0, dp(8));
        layout.addView(view);
        android.view.View divider = new android.view.View(this);
        divider.setBackgroundColor(Color.rgb(42, 58, 82));
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.MATCH_PARENT, dp(1));
        params.bottomMargin = dp(8);
        layout.addView(divider, params);
    }

    private void chooseGame() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_GAME);
    }

    private void chooseFirmware() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_FIRMWARE);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if ((requestCode == PICK_GAME || requestCode == PICK_FIRMWARE)
                && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                try {
                    getContentResolver().takePersistableUriPermission(
                        uri, Intent.FLAG_GRANT_READ_URI_PERMISSION);
                } catch (SecurityException ignored) {
                    // Some document providers do not grant persistable access.
                }
                if (requestCode == PICK_FIRMWARE) {
                    prefs.edit().putString("selected_firmware_uri", uri.toString()).apply();
                    selectedFirmware.setText(describeFirmwareFile(uri));
                } else {
                    prefs.edit().putString("selected_game_uri", uri.toString()).apply();
                    selectedGame.setText(describeSelectedFile(uri));
                }
            }
        }
    }

    private String describeFirmwareFile(Uri uri) {
        String displayName = uri.getLastPathSegment();
        long size = -1L;
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(uri,
                new String[] {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE},
                null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (nameIndex >= 0 && !cursor.isNull(nameIndex)) displayName = cursor.getString(nameIndex);
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) size = cursor.getLong(sizeIndex);
            }
        } catch (RuntimeException ignored) {
            // Some document providers do not expose metadata.
        } finally {
            if (cursor != null) cursor.close();
        }
        boolean pupName = displayName != null
            && displayName.toUpperCase(java.util.Locale.ROOT).endsWith(".PUP");
        String sizeText = size < 0 ? "Size unavailable"
            : String.format(java.util.Locale.ROOT, "%.2f MiB", size / (1024.0 * 1024.0));
        return "Selected: " + (displayName == null ? "Unknown file" : displayName)
            + "\nSize: " + sizeText
            + "\n" + (pupName ? "PUP extension detected (content not validated)."
                : "Warning: filename does not end in .PUP.")
            + "\nSaved for reference only. Firmware verification, installation, decryption and boot are not implemented.";
    }


    private void inspectLaunchSelectedGame() {
        String saved = prefs.getString("selected_game_uri", null);
        if (saved == null) {
            selectedGame.setText("Choose a PS3 game file first. No game is selected.");
            return;
        }
        Uri uri = Uri.parse(saved);
        byte[] header = new byte[64];
        int count = -1;
        try (java.io.InputStream input = getContentResolver().openInputStream(uri)) {
            if (input == null) {
                selectedGame.setText("Cannot open selected file. Choose it again and check file access.");
                return;
            }
            count = 0;
            while (count < header.length) {
                int n = input.read(header, count, header.length - count);
                if (n < 0) break;
                if (n == 0) continue;
                count += n;
            }
        } catch (java.io.IOException | SecurityException ex) {
            selectedGame.setText("Could not read the file header: " + ex.getClass().getSimpleName()
                + ". Choose the file again and retry.");
            return;
        }

        if (count >= 4 && header[0] == 0x7f && header[1] == 'E'
                && header[2] == 'L' && header[3] == 'F') {
            if (count >= 20 && header[4] == 2 && header[5] == 2
                    && header[18] == 0 && header[19] == 21) {
                tryRunSelectedElf(uri);
            } else {
                selectedGame.setText("ELF header detected, but it does not match the expected "
                    + "ELF64 big-endian PowerPC64 signature. Game was NOT launched.");
            }
        } else if (count >= 4 && header[0] == 0x7f && header[1] == 'P'
                && header[2] == 'K' && header[3] == 'G') {
            inspectPkg(uri);
        } else if (count >= 4 && header[0] == 'S' && header[1] == 'C'
                && header[2] == 'E' && header[3] == 0) {
            selectedGame.setText("SELF-style signature detected. Encrypted/signed executable handling "
                + "is not implemented, so this file cannot be launched.");
        } else {
            String discKind = detectDiscContainer(uri);
            if ("ISO 9660".equals(discKind)) {
                inspectIsoImage(uri);
            } else if ("UDF".equals(discKind)) {
                selectedGame.setText("UDF disc signature detected. This build's directory walker "
                    + "supports ISO 9660 only; UDF traversal and PS3 game boot are not implemented. "
                    + "Game was NOT launched.");
            } else {
                selectedGame.setText("No supported executable/container signature detected in the "
                    + "checked locations. PKG extraction and UDF traversal are not implemented. "
                    + "Game was NOT launched.");
            }
        }
    }

    private void inspectPkg(Uri uri) {
        selectedGame.setText("PS3 PKG signature detected. Validating bounded package header…");
        new Thread(() -> {
            try {
                PkgMetadataInspector.Info info =
                    PkgMetadataInspector.inspect(getContentResolver(), uri);
                final String report = PkgMetadataInspector.format(info);
                runOnUiThread(() -> selectedGame.setText(report));
            } catch (java.io.IOException | SecurityException ex) {
                runOnUiThread(() -> selectedGame.setText(
                    "PKG inspection stopped safely: " + ex.getMessage()
                    + "\\nNo package was extracted, installed, or launched."));
            } catch (RuntimeException ex) {
                runOnUiThread(() -> selectedGame.setText(
                    "PKG inspection failed safely (" + ex.getClass().getSimpleName()
                    + "). No package was extracted, installed, or launched."));
            }
        }, "ps3-pkg-header-inspection").start();
    }

    private void inspectIsoImage(Uri uri) {
        selectedGame.setText("ISO 9660 detected. Searching safely for PS3_GAME/USRDIR/EBOOT.BIN…");
        new Thread(() -> {
            try {
                Iso9660Locator.Result result = Iso9660Locator.findEboot(getContentResolver(), uri);
                if (result == null) {
                    runOnUiThread(() -> selectedGame.setText(
                        "ISO 9660 directory traversal completed, but PS3_GAME/USRDIR/EBOOT.BIN "
                        + "was not found. This may be a non-PS3 ISO, a UDF/Joliet layout, or an "
                        + "unsupported directory structure. Game was NOT launched."));
                    return;
                }
                if (result.size > 32L * 1024 * 1024) {
                    runOnUiThread(() -> selectedGame.setText(
                        "Found " + result.path + "\\nSize: " + result.size
                        + " bytes. It exceeds the prototype's 32 MiB executable limit; "
                        + "no file was loaded and no game was launched."));
                    return;
                }
                byte[] executable = Iso9660Locator.readEbootForRun(
                    getContentResolver(), uri, result);
                if (executable.length >= 4 && executable[0] == 0x7f
                        && executable[1] == 'E' && executable[2] == 'L'
                        && executable[3] == 'F') {
                    if (executable.length >= 20 && executable[4] == 2
                            && executable[5] == 2 && executable[18] == 0
                            && executable[19] == 21) {
                        runOnUiThread(() -> selectedGame.setText(
                            "Found an ELF64 big-endian PPC64 EBOOT.BIN at "
                            + result.path + ". Trying the bounded experimental interpreter…"));
                        final String runResult = nativeRunSelectedElf(executable);
                        runOnUiThread(() -> selectedGame.setText(
                            "ISO executable discovery: " + result.path + "\\n"
                            + "File size: " + result.size + " bytes\\n"
                            + "Interpreter result:\\n" + runResult
                            + "\\nThis is not a full PS3 game boot: firmware, imports, "
                            + "SPU and RSX are not implemented."));
                    } else {
                        runOnUiThread(() -> selectedGame.setText(
                            "Found EBOOT.BIN, but its ELF header is not the supported "
                            + "ELF64 big-endian PPC64 format. Path: " + result.path
                            + "\\nGame was NOT launched."));
                    }
                } else if (executable.length >= 4 && executable[0] == 'S'
                        && executable[1] == 'C' && executable[2] == 'E'
                        && executable[3] == 0) {
                    runOnUiThread(() -> selectedGame.setText(
                        "Found " + result.path + " (" + result.size + " bytes), but it has "
                        + "a SELF-style signature. Protected SELF execution is not implemented; "
                        + "the game was NOT launched."));
                } else {
                    runOnUiThread(() -> selectedGame.setText(
                        "Found " + result.path + ", but the file is not a recognized ELF or "
                        + "SELF executable. Game was NOT launched."));
                }
            } catch (java.io.IOException | SecurityException ex) {
                runOnUiThread(() -> selectedGame.setText(
                    "ISO inspection stopped safely: " + ex.getMessage()
                    + "\\nNo executable was launched."));
            } catch (RuntimeException ex) {
                runOnUiThread(() -> selectedGame.setText(
                    "ISO inspection failed safely (" + ex.getClass().getSimpleName()
                    + "). No executable was launched."));
            }
        }, "ps3-iso-eboot-discovery").start();
    }

    /**
     * Detects ISO 9660/UDF signatures at standard descriptor locations without
     * mounting, extracting, decrypting, or executing the image.
     */
    private String detectDiscContainer(Uri uri) {
        final int sectorSize = 2048;
        final int firstDescriptorSector = 16;
        final int descriptorCount = 32;
        try (java.io.InputStream input = getContentResolver().openInputStream(uri)) {
            if (input == null || !skipFully(input, (long) firstDescriptorSector * sectorSize)) {
                return null;
            }
            byte[] descriptor = new byte[6];
            for (int i = 0; i < descriptorCount; i++) {
                int count = 0;
                while (count < descriptor.length) {
                    int n = input.read(descriptor, count, descriptor.length - count);
                    if (n < 0) return null;
                    if (n == 0) continue;
                    count += n;
                }
                boolean isoType = (descriptor[0] >= 0 && descriptor[0] <= 3)
                    || (descriptor[0] & 0xff) == 255;
                if (isoType && descriptor[1] == 'C' && descriptor[2] == 'D'
                        && descriptor[3] == '0' && descriptor[4] == '0'
                        && descriptor[5] == '1') {
                    return "ISO 9660";
                }
                String identifier = new String(descriptor, 1, 5,
                    java.nio.charset.StandardCharsets.US_ASCII);
                if ("BEA01".equals(identifier) || "NSR02".equals(identifier)
                        || "NSR03".equals(identifier) || "TEA01".equals(identifier)) {
                    return "UDF";
                }
                if (!skipFully(input, sectorSize - descriptor.length)) return null;
            }
        } catch (java.io.IOException | SecurityException ignored) {
            // An unreadable or non-seekable provider is not treated as a valid image.
        }
        return null;
    }

    private boolean skipFully(java.io.InputStream input, long bytes)
            throws java.io.IOException {
        long remaining = bytes;
        while (remaining > 0) {
            long skipped = input.skip(remaining);
            if (skipped > 0) {
                remaining -= skipped;
            } else {
                if (input.read() < 0) return false;
                remaining--;
            }
        }
        return true;
    }


    private void tryRunSelectedElf(Uri uri) {
        selectedGame.setText("ELF candidate detected. Loading into the bounded guest runner…");
        new Thread(() -> {
            final int maxBytes = 32 * 1024 * 1024;
            byte[] fileBytes;
            try (java.io.InputStream input = getContentResolver().openInputStream(uri);
                 java.io.ByteArrayOutputStream output = new java.io.ByteArrayOutputStream()) {
                if (input == null) {
                    runOnUiThread(() -> selectedGame.setText(
                        "Cannot reopen the selected file. Choose it again and retry."));
                    return;
                }
                byte[] chunk = new byte[8192];
                int total = 0;
                int n;
                while ((n = input.read(chunk)) != -1) {
                    if (n == 0) continue;
                    total += n;
                    if (total > maxBytes) {
                        runOnUiThread(() -> selectedGame.setText(
                            "This ELF is larger than the prototype's 32 MiB file limit. "
                            + "No game was launched; larger files need a streaming loader."));
                        return;
                    }
                    output.write(chunk, 0, n);
                }
                fileBytes = output.toByteArray();
            } catch (java.io.IOException | SecurityException ex) {
                runOnUiThread(() -> selectedGame.setText(
                    "Could not read selected ELF: " + ex.getClass().getSimpleName()
                    + ". Choose the file again and retry."));
                return;
            }
            final String result = nativeRunSelectedElf(fileBytes);
            runOnUiThread(() -> selectedGame.setText(result));
        }, "ps3-elf-launch-test").start();
    }

    private String describeSelectedFile(Uri uri) {
        String displayName = uri.getLastPathSegment();
        long size = -1L;
        Cursor cursor = null;
        try {
            cursor = getContentResolver().query(uri,
                new String[] {OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE},
                null, null, null);
            if (cursor != null && cursor.moveToFirst()) {
                int nameIndex = cursor.getColumnIndex(OpenableColumns.DISPLAY_NAME);
                int sizeIndex = cursor.getColumnIndex(OpenableColumns.SIZE);
                if (nameIndex >= 0 && !cursor.isNull(nameIndex)) displayName = cursor.getString(nameIndex);
                if (sizeIndex >= 0 && !cursor.isNull(sizeIndex)) size = cursor.getLong(sizeIndex);
            }
        } catch (RuntimeException ignored) {
            // Document providers may not expose metadata; keep the URI fallback.
        } finally {
            if (cursor != null) cursor.close();
        }
        String lower = displayName == null ? "" : displayName.toLowerCase(java.util.Locale.ROOT);
        String format;
        if (lower.endsWith(".iso")) format = "ISO image (candidate disc image; not validated)";
        else if (lower.endsWith(".pkg")) format = "PKG package (candidate install package; not validated)";
        else if (lower.endsWith(".elf")) format = "ELF executable (candidate file; not validated)";
        else if (lower.endsWith(".self")) format = "SELF executable (candidate file; not validated)";
        else format = "Unknown extension (format not recognized)";
        String sizeText = size < 0 ? "Size: unavailable"
            : String.format(java.util.Locale.ROOT, "Size: %.2f MiB", size / (1024.0 * 1024.0));
        return "Selected: " + (displayName == null ? "Unknown file" : displayName)
            + "\n" + sizeText + "\nFormat hint: " + format
            + "\nSmall unencrypted ELF64/PPC64 files can be tried in the bounded experimental interpreter. PKG/SELF game boot is not implemented.";
    }

    private static native String nativeStatus();
    private static native String nativeRunSelectedElf(byte[] fileBytes);
    private static native String nativeRunBuiltinElfTest();

    static {
        System.loadLibrary("translator_bridge");
    }
}
