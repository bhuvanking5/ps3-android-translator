#ifndef PS3_TRANSLATOR_CONTROLLER_INPUT_H
#define PS3_TRANSLATOR_CONTROLLER_INPUT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t pressed_mask;
    float left_x;
    float left_y;
    float right_x;
    float right_y;
} ps3_controller_input;

/* Publish a normalized controller snapshot from the Android UI. */
void ps3_controller_input_publish(uint32_t pressed_mask,
    float left_x, float left_y, float right_x, float right_y);

/* Copy the latest input snapshot for a future emulator input adapter. */
ps3_controller_input ps3_controller_input_read(void);

#ifdef __cplusplus
}
#endif

#endif
