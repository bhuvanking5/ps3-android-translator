#ifndef PS3_PROTOTYPE_GUEST_IO_H
#define PS3_PROTOTYPE_GUEST_IO_H

#include <stdint.h>
#include "controller_input.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Synthetic prototype-only address map. These addresses are NOT documented
 * PS3 hardware addresses and must not be treated as hardware compatibility.
 */
#define PS3_PROTOTYPE_CONTROLLER_BASE 0xFFFF0000u
#define PS3_PROTOTYPE_CONTROLLER_SPAN 0x00000014u

typedef enum {
    PS3_GUEST_IO_OK = 0,
    PS3_GUEST_IO_INVALID_ARGUMENT,
    PS3_GUEST_IO_UNMAPPED_ADDRESS
} ps3_guest_io_status;

/* Reads a controller snapshot at a synthetic 32-bit guest address. */
ps3_guest_io_status ps3_prototype_guest_read32(
    const ps3_controller_report *report, uint32_t address, uint32_t *value);

#ifdef __cplusplus
}
#endif
#endif
