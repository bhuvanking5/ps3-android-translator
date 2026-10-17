#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guest_elf_loader.h"
#include "ppc_cpu.h"

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static void put16(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v >> 8); p[1] = (uint8_t)v;
}
static void put32(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v >> 24); p[1] = (uint8_t)(v >> 16);
    p[2] = (uint8_t)(v >> 8); p[3] = (uint8_t)v;
}
static void put64(uint8_t *p, uint64_t v) {
    put32(p, (uint32_t)(v >> 32)); put32(p + 4, (uint32_t)v);
}

int main(void) {
    uint8_t file[144] = {0};
    uint8_t ram[64];
    memset(ram, 0xa5, sizeof(ram));
    ps3_guest_memory memory;
    uint32_t entry = 0u;
    ps3_guest_memory_init(&memory);

    memcpy(file, "\x7f""ELF", 4);
    file[4] = 2u; file[5] = 2u; file[6] = 1u;
    put16(file + 16, 2u); put16(file + 18, 21u); put32(file + 20, 1u);
    put64(file + 24, 0x1000u); put64(file + 32, 64u);
    put16(file + 52, 64u); put16(file + 54, 56u); put16(file + 56, 1u);

    put32(file + 64, 1u);
    put32(file + 68, 5u);
    put64(file + 72, 120u);
    put64(file + 80, 0x1000u);
    put64(file + 88, 0x1000u);
    put64(file + 96, 20u);
    put64(file + 104, 24u);
    put64(file + 112, 4u);
    put32(file + 120, 0x38600007u); /* addi r3,0,7 */
    put32(file + 124, 0x38802000u); /* addi r4,0,0x2000 */
    put32(file + 128, 0x90640000u); /* stw r3,0(r4) */
    put32(file + 132, 0x80a40000u); /* lwz r5,0(r4) */
    put32(file + 136, 0x48000000u); /* b . */


    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x1000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_OK);
    CHECK(entry == 0x1000u);
    CHECK(memory.count == 1u);
    CHECK(ram[0] == 0x38u && ram[1] == 0x60u &&
          ram[2] == 0x00u && ram[3] == 0x07u);
    CHECK(ram[16] == 0x48u && ram[17] == 0u &&
          ram[18] == 0u && ram[19] == 0u);
    CHECK(ram[20] == 0u && ram[21] == 0u &&
          ram[22] == 0u && ram[23] == 0u);
    uint32_t word = 0u;
    CHECK(ps3_guest_memory_read32(&memory, 0x1000u, &word));
    CHECK(word == 0x38600007u);
    CHECK(!ps3_guest_memory_write32(&memory, 0x1000u, 1u));
    uint8_t data[4] = {0};
    CHECK(ps3_guest_memory_map(&memory, 0x2000u, sizeof(data), data,
        PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE));
    uint64_t registers[32] = {0};
    size_t steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, entry, registers, 8u, &steps) ==
        PPC_CPU_STEP_LIMIT);
    CHECK(steps == 8u);
    CHECK(registers[3] == 7u);
    CHECK(registers[5] == 7u);
    CHECK(data[0] == 0u && data[1] == 0u &&
          data[2] == 0u && data[3] == 7u);

    ps3_guest_memory_init(&memory);
    file[71] = 4u;
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x1000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);

    puts("Native ELF loader and entry-point execution tests passed.");
    return 0;
}
