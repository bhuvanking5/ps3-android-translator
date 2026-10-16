package org.ps3translator.prototype;

import java.util.Locale;

/**
 * Thread-safe virtual controller state published to a native input mailbox.
 * The native mailbox is ready for an emulator adapter, but no PS3 emulator
 * currently consumes it.
 */
public final class ControllerInputState {
    private static final String[] BUTTONS = {
        "UP", "DOWN", "LEFT", "RIGHT", "SELECT",
        "TRIANGLE", "SQUARE", "CIRCLE", "CROSS",
        "L1", "L2", "R1", "R2", "L3", "R3"
    };
    private static int pressedMask;
    private static float leftX, leftY, rightX, rightY;

    private ControllerInputState() {}

    public static synchronized void setButton(String name, boolean pressed) {
        int index = indexOf(name);
        if (index < 0) return;
        int bit = 1 << index;
        pressedMask = pressed ? (pressedMask | bit) : (pressedMask & ~bit);
        publish();
    }

    public static synchronized void setLeftAxes(float x, float y) {
        leftX = clamp(x);
        leftY = clamp(y);
        publish();
    }

    public static synchronized void setRightAxes(float x, float y) {
        rightX = clamp(x);
        rightY = clamp(y);
        publish();
    }

    public static synchronized boolean isPressed(String name) {
        int index = indexOf(name);
        return index >= 0 && (pressedMask & (1 << index)) != 0;
    }

    public static synchronized float[] getAxesSnapshot() {
        return new float[] { leftX, leftY, rightX, rightY };
    }

    public static synchronized void releaseAll() {
        pressedMask = 0;
        leftX = leftY = rightX = rightY = 0f;
        publish();
    }

    public static synchronized String snapshot() {
        StringBuilder buttons = new StringBuilder();
        for (int i = 0; i < BUTTONS.length; i++) {
            if ((pressedMask & (1 << i)) != 0) {
                if (buttons.length() > 0) buttons.append(", ");
                buttons.append(BUTTONS[i]);
            }
        }
        if (buttons.length() == 0) buttons.append("none");
        return String.format(Locale.ROOT,
            "Buttons: %s\nLeft: %.2f, %.2f | Right: %.2f, %.2f",
            buttons, leftX, leftY, rightX, rightY);
    }

    private static void publish() {
        nativePublish(pressedMask, leftX, leftY, rightX, rightY);
    }

    /* Native mailbox publisher implemented by translator_bridge. */
    private static native void nativePublish(
        int pressedMask, float leftX, float leftY, float rightX, float rightY);

    private static int indexOf(String name) {
        for (int i = 0; i < BUTTONS.length; i++) {
            if (BUTTONS[i].equals(name)) return i;
        }
        return -1;
    }

    private static float clamp(float value) {
        if (Float.isNaN(value) || Float.isInfinite(value)) return 0f;
        return Math.max(-1f, Math.min(1f, value));
    }
}
