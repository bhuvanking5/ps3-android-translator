#ifndef PS3_TRANSLATOR_CONTROLLER_INPUT_H
#define PS3_TRANSLATOR_CONTROLLER_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Button bit positions mirror ControllerInputState.java. */
enum {
    PS3_INPUT_UP = 1u << 0, PS3_INPUT_DOWN = 1u << 1,
    PS3_INPUT_LEFT = 1u << 2, PS3_INPUT_RIGHT = 1u << 3,
    PS3_INPUT_SELECT = 1u << 4, PS3_INPUT_TRIANGLE = 1u << 5,
    PS3_INPUT_SQUARE = 1u << 6, PS3_INPUT_CIRCLE = 1u << 7,
    PS3_INPUT_CROSS = 1u << 8, PS3_INPUT_L1 = 1u << 9,
    PS3_INPUT_L2 = 1u << 10, PS3_INPUT_R1 = 1u << 11,
    PS3_INPUT_R2 = 1u << 12, PS3_INPUT_L3 = 1u << 13,
    PS3_INPUT_R3 = 1u << 14
};

typedef struct {
    uint32_t pressed_mask;
    float left_x, left_y, right_x, right_y;
} ps3_controller_input;

/* Single publisher is expected (the Java state methods serialize updates).
 * Readers retry while a publication is in progress, preserving one coherent
 * button-and-axis snapshot. Generic report axis 0=negative, 128=center, 255=positive.
 * This is an adapter contract, not a claim of exact hardware protocol parity. */
typedef struct {
    uint32_t buttons;
    uint8_t left_x, left_y, right_x, right_y;
} ps3_controller_report;

/* Stable 32-bit word layout for a future guest-memory/MMIO adapter. */
typedef enum {
    PS3_CONTROLLER_WORD_BUTTONS = 0,
    PS3_CONTROLLER_WORD_LEFT_X = 1,
    PS3_CONTROLLER_WORD_LEFT_Y = 2,
    PS3_CONTROLLER_WORD_RIGHT_X = 3,
    PS3_CONTROLLER_WORD_RIGHT_Y = 4,
    PS3_CONTROLLER_WORD_COUNT = 5
} ps3_controller_word_index;

typedef enum {
    PS3_CONTROLLER_WORD_OK = 0,
    PS3_CONTROLLER_WORD_INVALID_ARGUMENT,
    PS3_CONTROLLER_WORD_INVALID_INDEX
} ps3_controller_word_status;

void ps3_controller_input_publish(uint32_t pressed_mask,
    float left_x, float left_y, float right_x, float right_y);
ps3_controller_input ps3_controller_input_read(void);
ps3_controller_report ps3_controller_input_read_report(void);
/* Reads one report field as a zero-extended 32-bit word. */
ps3_controller_word_status ps3_controller_report_read_word(
    const ps3_controller_report *report, uint32_t word_index, uint32_t *value);

#ifdef __cplusplus
}
#endif
#endif
