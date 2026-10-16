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

    ps3_controller_input_publish(0u, 2.0f, -2.0f, NAN, INFINITY);
    state = ps3_controller_input_read();
    CHECK(state.pressed_mask == 0u);
    CHECK(state.left_x == 0.0f && state.left_y == 0.0f);
    CHECK(state.right_x == 0.0f && state.right_y == 0.0f);

    puts("Native controller input mailbox tests passed.");
    return 0;
}
