#include <math.h>
#include <stdio.h>
#include "controller_input.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    ps3_controller_input_publish(0x15u, -1.0f, 0.25f, 1.0f, -0.5f);
    ps3_controller_input state = ps3_controller_input_read();
    CHECK(state.pressed_mask == 0x15u);
    CHECK(state.left_x == -1.0f);
    CHECK(state.left_y == 0.25f);
    CHECK(state.right_x == 1.0f);
    CHECK(state.right_y == -0.5f);

    ps3_controller_report report = ps3_controller_input_read_report();
    CHECK(report.buttons == 0x15u);
    CHECK(report.left_x == 0u);
    CHECK(report.left_y == 159u);
    CHECK(report.right_x == 255u);
    CHECK(report.right_y == 64u);

    /* The stable word adapter zero-extends axes and preserves button flags. */
    uint32_t word = 0xFFFFFFFFu;
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_BUTTONS, &word) == PS3_CONTROLLER_WORD_OK);
    CHECK(word == 0x15u);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_LEFT_X, &word) == PS3_CONTROLLER_WORD_OK);
    CHECK(word == 0u);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_LEFT_Y, &word) == PS3_CONTROLLER_WORD_OK);
    CHECK(word == 159u);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_RIGHT_X, &word) == PS3_CONTROLLER_WORD_OK);
    CHECK(word == 255u);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_RIGHT_Y, &word) == PS3_CONTROLLER_WORD_OK);
    CHECK(word == 64u);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_COUNT, &word) ==
        PS3_CONTROLLER_WORD_INVALID_INDEX);
    CHECK(ps3_controller_report_read_word(0, PS3_CONTROLLER_WORD_BUTTONS,
        &word) == PS3_CONTROLLER_WORD_INVALID_ARGUMENT);
    CHECK(ps3_controller_report_read_word(&report,
        PS3_CONTROLLER_WORD_BUTTONS, 0) ==
        PS3_CONTROLLER_WORD_INVALID_ARGUMENT);

    ps3_controller_input_publish(0u, 2.0f, -2.0f, NAN, INFINITY);
    state = ps3_controller_input_read();
    CHECK(state.pressed_mask == 0u);
    CHECK(state.left_x == 0.0f && state.left_y == 0.0f);
    CHECK(state.right_x == 0.0f && state.right_y == 0.0f);
    report = ps3_controller_input_read_report();
    CHECK(report.buttons == 0u);
    CHECK(report.left_x == 128u && report.left_y == 128u);
    CHECK(report.right_x == 128u && report.right_y == 128u);

    puts("Native controller input mailbox tests passed.");
    return 0;
}
