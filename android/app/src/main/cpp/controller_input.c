#include "controller_input.h"
#include <stdatomic.h>
#include <string.h>

/*
 * Prototype input mailbox. It is deliberately not wired to game execution:
 * the current app has no PS3 emulator input loop.
 */
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
    atomic_store_explicit(&g_left_x_bits, float_bits(clamp_axis(left_x)), memory_order_relaxed);
    atomic_store_explicit(&g_left_y_bits, float_bits(clamp_axis(left_y)), memory_order_relaxed);
    atomic_store_explicit(&g_right_x_bits, float_bits(clamp_axis(right_x)), memory_order_relaxed);
    atomic_store_explicit(&g_right_y_bits, float_bits(clamp_axis(right_y)), memory_order_relaxed);
    atomic_store_explicit(&g_pressed_mask, pressed_mask, memory_order_release);
}

ps3_controller_input ps3_controller_input_read(void) {
    ps3_controller_input state;
    state.pressed_mask = atomic_load_explicit(&g_pressed_mask, memory_order_acquire);
    state.left_x = bits_float(atomic_load_explicit(&g_left_x_bits, memory_order_relaxed));
    state.left_y = bits_float(atomic_load_explicit(&g_left_y_bits, memory_order_relaxed));
    state.right_x = bits_float(atomic_load_explicit(&g_right_x_bits, memory_order_relaxed));
    state.right_y = bits_float(atomic_load_explicit(&g_right_y_bits, memory_order_relaxed));
    return state;
}
