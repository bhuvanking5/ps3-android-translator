#include "controller_input.h"
#include <stdatomic.h>
#include <string.h>

/*
 * Prototype input mailbox. It is deliberately not wired to game execution:
 * the current app has no PS3 emulator input loop.
 */
/* Single-writer sequence counter: odd while publishing, even when stable. */
static _Atomic uint32_t g_sequence;
static _Atomic uint32_t g_pressed_mask;
static _Atomic uint32_t g_left_x_bits;
static _Atomic uint32_t g_left_y_bits;
static _Atomic uint32_t g_right_x_bits;
static _Atomic uint32_t g_right_y_bits;

static uint32_t float_bits(float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static float bits_float(uint32_t bits) {
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static float clamp_axis(float value) {
    if (!(value >= -1.0f && value <= 1.0f)) return 0.0f;
    return value;
}

void ps3_controller_input_publish(uint32_t pressed_mask,
    float left_x, float left_y, float right_x, float right_y) {
    atomic_fetch_add_explicit(&g_sequence, 1u, memory_order_acq_rel);
    atomic_store_explicit(&g_left_x_bits, float_bits(clamp_axis(left_x)), memory_order_relaxed);
    atomic_store_explicit(&g_left_y_bits, float_bits(clamp_axis(left_y)), memory_order_relaxed);
    atomic_store_explicit(&g_right_x_bits, float_bits(clamp_axis(right_x)), memory_order_relaxed);
    atomic_store_explicit(&g_right_y_bits, float_bits(clamp_axis(right_y)), memory_order_relaxed);
    atomic_store_explicit(&g_pressed_mask, pressed_mask, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_sequence, 1u, memory_order_release);
}

ps3_controller_input ps3_controller_input_read(void) {
    ps3_controller_input state;
    uint32_t before, after;
    do {
        before = atomic_load_explicit(&g_sequence, memory_order_acquire);
        if (before & 1u) continue;
        state.pressed_mask = atomic_load_explicit(&g_pressed_mask, memory_order_relaxed);
        state.left_x = bits_float(atomic_load_explicit(&g_left_x_bits, memory_order_relaxed));
        state.left_y = bits_float(atomic_load_explicit(&g_left_y_bits, memory_order_relaxed));
        state.right_x = bits_float(atomic_load_explicit(&g_right_x_bits, memory_order_relaxed));
        state.right_y = bits_float(atomic_load_explicit(&g_right_y_bits, memory_order_relaxed));
        after = atomic_load_explicit(&g_sequence, memory_order_acquire);
    } while (before != after || (after & 1u));
    return state;
}

static uint8_t axis_to_u8(float value) {
    const float normalized = clamp_axis(value);
    return (uint8_t)((normalized + 1.0f) * 127.5f + 0.5f);
}

/* Convert the UI mailbox into a stable, generic emulator-facing report. */
ps3_controller_report ps3_controller_input_read_report(void) {
    const ps3_controller_input input = ps3_controller_input_read();
    ps3_controller_report report;
    report.buttons = input.pressed_mask;
    report.left_x = axis_to_u8(input.left_x);
    report.left_y = axis_to_u8(input.left_y);
    report.right_x = axis_to_u8(input.right_x);
    report.right_y = axis_to_u8(input.right_y);
    return report;
}
