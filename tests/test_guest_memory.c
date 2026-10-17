#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guest_memory.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

int main(void) {
    ps3_guest_memory memory;
    uint8_t ram[16] = {0};
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x1000u, sizeof(ram), ram,
        PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE));
    CHECK(!ps3_guest_memory_map(&memory, 0x1008u, 8u, ram,
        PS3_GUEST_MEMORY_READ));
    CHECK(!ps3_guest_memory_map(&memory, 0xfffffffcu, 8u, ram,
        PS3_GUEST_MEMORY_READ));
    CHECK(!ps3_guest_memory_map(&memory, 0x2000u, 4u, ram, 0u));

    CHECK(ps3_guest_memory_write32(&memory, 0x1004u, 0x1234abcdU));
    CHECK(ram[4] == 0x12u && ram[5] == 0x34u &&
          ram[6] == 0xabu && ram[7] == 0xcdu);
    uint32_t value = 0u;
    CHECK(ps3_guest_memory_read32(&memory, 0x1004u, &value));
    CHECK(value == 0x1234abcdU);
    CHECK(!ps3_guest_memory_read32(&memory, 0x100du, &value));
    CHECK(!ps3_guest_memory_read32(&memory, 0x1010u, &value));
    CHECK(!ps3_guest_memory_read32(&memory, 0x2000u, &value));
    CHECK(!ps3_guest_memory_read32(&memory, 0x1000u, 0));
    puts("Guest memory tests passed.");
    return 0;
}
