package org.ps3translator.prototype;

import android.app.Activity;
import android.os.Bundle;
import android.graphics.Color;
import android.view.Gravity;
import android.view.MotionEvent;
import android.view.View;
import android.widget.Button;
import android.widget.LinearLayout;
import android.widget.TextView;

/**
 * Touch-controller layout prototype. It reports button presses locally;
 * it is not yet wired to a PS3 input/emulation backend.
 */
public final class VirtualControllerActivity extends Activity {
    private TextView status;

    @Override
    protected void onCreate(Bundle state) {
        super.onCreate(state);
        LinearLayout root = new LinearLayout(this);
        root.setOrientation(LinearLayout.VERTICAL);
        root.setPadding(16, 20, 16, 20);
        root.setBackgroundColor(Color.rgb(18, 22, 32));

        TextView title = new TextView(this);
        title.setText("Virtual Controller");
        title.setTextSize(25);
        title.setTextColor(Color.WHITE);
        title.setGravity(Gravity.CENTER);
        root.addView(title);

        TextView note = new TextView(this);
        note.setText("Touch buttons to test the layout. Input is not connected to a PS3 game yet.");
        note.setTextSize(14);
        note.setTextColor(Color.rgb(255, 205, 130));
        note.setPadding(0, 12, 0, 16);
        root.addView(note);

        status = new TextView(this);
        status.setText("Input: idle");
        status.setTextSize(18);
        status.setTextColor(Color.rgb(130, 230, 160));
        status.setGravity(Gravity.CENTER);
        status.setPadding(0, 8, 0, 16);
        root.addView(status);

        LinearLayout upper = row(root);
        addTouchButton(upper, "L1", "L1");
        addTouchButton(upper, "L2", "L2");
        addTouchButton(upper, "R2", "R2");
        addTouchButton(upper, "R1", "R1");

        LinearLayout middle = row(root);
        LinearLayout dpad = new LinearLayout(this);
        dpad.setOrientation(LinearLayout.VERTICAL);
        dpad.setGravity(Gravity.CENTER);
        addTouchButton(dpad, "▲", "UP");
        LinearLayout horizontal = new LinearLayout(this);
        horizontal.setGravity(Gravity.CENTER);
        addTouchButton(horizontal, "◀", "LEFT");
        addTouchButton(horizontal, "●", "SELECT");
        addTouchButton(horizontal, "▶", "RIGHT");
        dpad.addView(horizontal);
        addTouchButton(dpad, "▼", "DOWN");
        middle.addView(dpad, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));

        LinearLayout actions = new LinearLayout(this);
        actions.setOrientation(LinearLayout.VERTICAL);
        actions.setGravity(Gravity.CENTER);
        addTouchButton(actions, "△", "TRIANGLE");
        LinearLayout actionMid = new LinearLayout(this);
        addTouchButton(actionMid, "□", "SQUARE");
        addTouchButton(actionMid, "○", "CIRCLE");
        actions.addView(actionMid);
        addTouchButton(actions, "✕", "CROSS");
        middle.addView(actions, new LinearLayout.LayoutParams(0, LinearLayout.LayoutParams.WRAP_CONTENT, 1));

        LinearLayout sticks = row(root);
        addTouchButton(sticks, "Left stick", "LEFT STICK");
        addTouchButton(sticks, "L3", "L3");
        addTouchButton(sticks, "R3", "R3");
        addTouchButton(sticks, "Right stick", "RIGHT STICK");

        Button back = new Button(this);
        back.setText("Back to main screen");
        back.setOnClickListener(v -> finish());
        root.addView(back);
        setContentView(root);
    }

    private LinearLayout row(LinearLayout parent) {
        LinearLayout row = new LinearLayout(this);
        row.setGravity(Gravity.CENTER);
        row.setPadding(0, 5, 0, 5);
        parent.addView(row);
        return row;
    }

    private void addTouchButton(LinearLayout parent, String label, String inputName) {
        Button button = new Button(this);
        button.setText(label);
        button.setTextSize(14);
        button.setAllCaps(false);
        LinearLayout.LayoutParams params = new LinearLayout.LayoutParams(
            0, LinearLayout.LayoutParams.WRAP_CONTENT, 1);
        params.setMargins(3, 3, 3, 3);
        parent.addView(button, params);
        button.setOnTouchListener((View view, MotionEvent event) -> {
            if (event.getAction() == MotionEvent.ACTION_DOWN) {
                status.setText("Input: " + inputName + " pressed");
                view.setAlpha(0.65f);
                return true;
            }
            if (event.getAction() == MotionEvent.ACTION_UP
                    || event.getAction() == MotionEvent.ACTION_CANCEL) {
                status.setText("Input: idle");
                view.setAlpha(1.0f);
                return true;
            }
            return true;
        });
    }
}
