#include <stdio.h>
#include "prototype_guest_io.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    const ps3_controller_report report = {
        .buttons = 0x1234u,
        .left_x = 0u,
        .left_y = 128u,
        .right_x = 255u,
        .right_y = 64u
    };
    uint32_t value = 0u;
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE, &value) == PS3_GUEST_IO_OK);
    CHECK(value == 0x1234u);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 4u, &value) == PS3_GUEST_IO_OK);
    CHECK(value == 0u);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 8u, &value) == PS3_GUEST_IO_OK);
    CHECK(value == 128u);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 12u, &value) == PS3_GUEST_IO_OK);
    CHECK(value == 255u);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 16u, &value) == PS3_GUEST_IO_OK);
    CHECK(value == 64u);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 20u, &value) ==
        PS3_GUEST_IO_UNMAPPED_ADDRESS);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE + 2u, &value) ==
        PS3_GUEST_IO_UNMAPPED_ADDRESS);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE - 4u, &value) ==
        PS3_GUEST_IO_UNMAPPED_ADDRESS);
    CHECK(ps3_prototype_guest_read32(0,
        PS3_PROTOTYPE_CONTROLLER_BASE, &value) ==
        PS3_GUEST_IO_INVALID_ARGUMENT);
    CHECK(ps3_prototype_guest_read32(&report,
        PS3_PROTOTYPE_CONTROLLER_BASE, 0) ==
        PS3_GUEST_IO_INVALID_ARGUMENT);
    puts("Prototype guest I/O tests passed.");
    return 0;
}
