package org.ps3translator.prototype;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.view.Gravity;
import android.widget.LinearLayout;
import android.widget.TextView;

public final class MainActivity extends Activity {
    private static final String TAG = "PS3TranslatorPrototype";

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        layout.setGravity(Gravity.CENTER);
        layout.setPadding(28, 28, 28, 28);
        layout.setBackgroundColor(Color.rgb(18, 22, 32));

        TextView title = new TextView(this);
        title.setText("PS3 → Android");
        title.setTextSize(28);
        title.setTextColor(Color.WHITE);
        title.setGravity(Gravity.CENTER);

        TextView status = new TextView(this);
        status.setTextSize(17);
        status.setTextColor(Color.rgb(180, 220, 255));
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, 24, 0, 0);
        status.setText(nativeStatus());

        TextView note = new TextView(this);
        note.setTextSize(14);
        note.setTextColor(Color.LTGRAY);
        note.setGravity(Gravity.CENTER);
        note.setPadding(0, 20, 0, 0);
        note.setText("Research prototype only. PS3 games are not supported yet.");
        layout.addView(title);
        layout.addView(status);
        layout.addView(note);
        setContentView(layout);
    }

    private static native String nativeStatus();

    static {
        System.loadLibrary("translator_bridge");
    }
}
