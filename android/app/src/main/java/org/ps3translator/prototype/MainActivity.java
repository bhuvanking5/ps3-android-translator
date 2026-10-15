package org.ps3translator.prototype;

import android.app.Activity;
import android.os.Bundle;
import android.content.Intent;
import android.content.SharedPreferences;
import android.graphics.Color;
import android.net.Uri;
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
    private static final String PREFS = "prototype_settings";
    private LinearLayout layout;
    private TextView selectedGame;
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
        layout.setPadding(24, 28, 24, 28);
        layout.setBackgroundColor(Color.rgb(18, 22, 32));
        scroll.addView(layout);

        TextView title = text("PS3 → Android", 28, Color.WHITE);
        title.setGravity(Gravity.CENTER);
        layout.addView(title);
        TextView status = text(nativeStatus(), 16, Color.rgb(180, 220, 255));
        status.setPadding(0, 18, 0, 16);
        layout.addView(status);

        heading("Games");
        Button addGame = new Button(this);
        addGame.setText("Choose game file");
        addGame.setOnClickListener(v -> chooseGame());
        layout.addView(addGame);
        selectedGame = text("No file selected. Choosing a file does not launch it.", 14, Color.LTGRAY);
        selectedGame.setPadding(0, 8, 0, 8);
        layout.addView(selectedGame);

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

        TextView caveat = text("Prototype controls only: these options are saved, but they do not change game rendering yet. Game files are selected for UI testing only and are not read or executed. PS3 emulation is not implemented.", 13, Color.LTGRAY);
        caveat.setPadding(0, 18, 0, 0);
        layout.addView(caveat);
        setContentView(scroll);
    }

    private TextView text(String value, int size, int color) {
        TextView view = new TextView(this);
        view.setText(value);
        view.setTextSize(size);
        view.setTextColor(color);
        return view;
    }

    private void heading(String value) {
        TextView view = text(value, 20, Color.WHITE);
        view.setPadding(0, 18, 0, 8);
        layout.addView(view);
    }

    private void chooseGame() {
        Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
        intent.addCategory(Intent.CATEGORY_OPENABLE);
        intent.setType("*/*");
        startActivityForResult(intent, PICK_GAME);
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == PICK_GAME && resultCode == RESULT_OK && data != null) {
            Uri uri = data.getData();
            if (uri != null) {
                selectedGame.setText("Selected file: " + uri.getLastPathSegment()
                    + "\nSelected for interface testing only; this prototype cannot launch PS3 games.");
            }
        }
    }

    private static native String nativeStatus();

    static {
        System.loadLibrary("translator_bridge");
    }
}
