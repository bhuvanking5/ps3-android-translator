#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "guest_elf_loader.h"
#include "ppc_cpu.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (0)

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
    uint8_t file[128] = {0};
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

    put32(file + 64, 1u);        /* PT_LOAD */
    put32(file + 68, 5u);        /* PF_R | PF_X */
    put64(file + 72, 120u);      /* file offset */
    put64(file + 80, 0x1000u);   /* virtual address */
    put64(file + 88, 0x1000u);
    put64(file + 96, 4u);        /* file size */
    put64(file + 104, 8u);       /* memory size, including BSS */
    put64(file + 112, 4u);
    file[120] = 0x38u; file[121] = 0x60u; file[122] = 0x00u; file[123] = 0x07u;\n    file[124] = 0x48u; file[125] = 0x00u; file[126] = 0x00u; file[127] = 0x00u; /* b to self */

    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x1000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_OK);
    CHECK(entry == 0x1000u);
    CHECK(memory.count == 1u);
    CHECK(ram[0] == 0x38u && ram[1] == 0x60u &&
          ram[2] == 0x00u && ram[3] == 0x07u);
    CHECK(ram[4] == 0u && ram[5] == 0u && ram[6] == 0u && ram[7] == 0u);
    uint32_t word = 0u;
    CHECK(ps3_guest_memory_read32(&memory, 0x1000u, &word));
    CHECK(word == 0x38600007u);
    CHECK(!ps3_guest_memory_write32(&memory, 0x1000u, 1u));\n    uint64_t registers[32] = {0};\n    size_t steps = 0u;\n    CHECK(ppc_cpu_run_guest(&memory, entry, registers, 8u, &steps) ==\n        PPC_CPU_STEP_LIMIT);\n    CHECK(steps == 8u);\n    CHECK(registers[3] == 7u);

    ps3_guest_memory_init(&memory);
    file[71] = 4u; /* read-only segment: entry is not executable */
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x1000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);

    puts("Native ELF loader tests passed.");
    return 0;
}
