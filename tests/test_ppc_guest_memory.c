#include <stdint.h>
#include <stdio.h>
#include "guest_memory.h"
#include "ppc_cpu.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static uint32_t addi(unsigned rt, unsigned ra, int imm) {
    return (14u << 26) | (rt << 21) | (ra << 16) |
           ((uint32_t)imm & 0xffffu);
}

int main(void) {
    ps3_guest_memory memory;
    uint8_t ram[16] = {0};
    uint64_t registers[32] = {0};
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x1000u, sizeof(ram), ram,
        PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE));

    const uint32_t program[] = {
        addi(3u, 0u, 0x1000), /* r3 = guest RAM base */
        addi(4u, 0u, 0x1234), /* r4 = value to store */
        (36u << 26) | (4u << 21) | (3u << 16) | 4u, /* stw r4,4(r3) */
        (32u << 26) | (5u << 21) | (3u << 16) | 4u  /* lwz r5,4(r3) */
    };
    CHECK(ppc_cpu_run_with_memory(program, 4u, registers,
        ps3_guest_memory_read32, ps3_guest_memory_write32, &memory) == PPC_CPU_OK);
    CHECK(registers[5] == 0x1234u);
    CHECK(ram[4] == 0u && ram[5] == 0u && ram[6] == 0x12u && ram[7] == 0x34u);

    const uint32_t unmapped[] = {
        addi(3u, 0u, 0x2000),
        (32u << 26) | (5u << 21) | (3u << 16)
    };
    CHECK(ppc_cpu_run_with_memory(unmapped, 2u, registers,
        ps3_guest_memory_read32, ps3_guest_memory_write32, &memory) ==
        PPC_CPU_MEMORY_ACCESS_ERROR);
    puts("PowerPC guest memory integration tests passed.");
    return 0;
}
