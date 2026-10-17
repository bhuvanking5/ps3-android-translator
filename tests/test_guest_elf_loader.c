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
    uint8_t file[152] = {0};
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
    put64(file + 96, 32u);
    put64(file + 104, 36u);
    put64(file + 112, 4u);
    put32(file + 120, 0x38600007u); /* addi r3,0,7 */
    put32(file + 124, 0x38802000u); /* addi r4,0,0x2000 */
    put32(file + 128, 0x90640000u); /* stw r3,0(r4) */
    put32(file + 132, 0x80a40000u); /* lwz r5,0(r4) */
    put32(file + 136, 0x2c050007u); /* cmpwi cr0,r5,7 */
    put32(file + 140, 0x41820008u); /* beq cr0,+8: skip next instruction */
    put32(file + 144, 0x38c00063u); /* addi r6,0,99 (must be skipped) */
    put32(file + 148, 0x4e800020u); /* blr: clean prototype top-level return */


    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x1000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_OK);
    CHECK(entry == 0x1000u);
    CHECK(memory.count == 1u);
    CHECK(ram[0] == 0x38u && ram[1] == 0x60u &&
          ram[2] == 0x00u && ram[3] == 0x07u);
    CHECK(ram[28] == 0x4eu && ram[29] == 0x80u &&
          ram[30] == 0u && ram[31] == 0x20u);
    CHECK(ram[32] == 0u && ram[33] == 0u &&
          ram[34] == 0u && ram[35] == 0u);
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
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 7u);
    CHECK(registers[3] == 7u);
    CHECK(registers[4] == 0x2000u);
    CHECK(registers[5] == 7u);
    CHECK(registers[6] == 0u); /* conditional branch skipped addi r6,0,99 */
    CHECK(data[0] == 0u && data[1] == 0u &&
          data[2] == 0u && data[3] == 7u);

    /* A load from unmapped guest data must stop with a memory fault. */
    uint8_t fault_code[8] = {0};
    put32(fault_code, 0x38805000u);     /* addi r4,0,0x5000 */
    put32(fault_code + 4, 0x80640000u); /* lwz r3,0(r4) */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x6000u, sizeof(fault_code),
        fault_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 99u;
    CHECK(ppc_cpu_run_guest(&memory, 0x6000u, registers, 4u, &steps) ==
        PPC_CPU_MEMORY_ACCESS_ERROR);
    CHECK(steps == 2u);

    /* A store to read-only guest data must also fail safely. */
    uint8_t store_code[12] = {0};
    uint8_t readonly_data[4] = {0};
    put32(store_code, 0x38600009u);      /* addi r3,0,9 */
    put32(store_code + 4, 0x38805000u);  /* addi r4,0,0x5000 */
    put32(store_code + 8, 0x90640000u);  /* stw r3,0(r4) */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x6000u, sizeof(store_code),
        store_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    CHECK(ps3_guest_memory_map(&memory, 0x5000u, sizeof(readonly_data),
        readonly_data, PS3_GUEST_MEMORY_READ));
    memset(registers, 0, sizeof(registers));
    steps = 99u;
    CHECK(ppc_cpu_run_guest(&memory, 0x6000u, registers, 4u, &steps) ==
        PPC_CPU_MEMORY_ACCESS_ERROR);
    CHECK(steps == 3u);
    CHECK(readonly_data[0] == 0u && readonly_data[1] == 0u &&
          readonly_data[2] == 0u && readonly_data[3] == 0u);

    /* Unsupported guest opcodes must stop instead of being silently skipped. */
    uint8_t invalid_instruction[4] = {0u, 0u, 0u, 0u};
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x3000u, sizeof(invalid_instruction),
        invalid_instruction, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 99u;
    CHECK(ppc_cpu_run_guest(&memory, 0x3000u, registers, 4u, &steps) ==
        PPC_CPU_UNSUPPORTED_INSTRUCTION);
    CHECK(steps == 1u);

    /* The ELF-entry guest runner must reject record-form ops it cannot update. */
    uint8_t record_form_code[4] = {0x7cu, 0xa3u, 0x22u, 0x15u}; /* add. r5,r3,r4 */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x3000u, sizeof(record_form_code),
        record_form_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 99u;
    CHECK(ppc_cpu_run_guest(&memory, 0x3000u, registers, 4u, &steps) ==
        PPC_CPU_UNSUPPORTED_INSTRUCTION);
    CHECK(steps == 1u);


    /* Shifted logical immediates must behave the same in the actual ELF runner. */
    uint8_t shifted_logic_code[12] = {0};
    put32(shifted_logic_code, 0x38601234u);       /* addi r3,0,0x1234 */
    put32(shifted_logic_code + 4, 0x64A4ABCDu);   /* oris r4,r5,0xABCD (patched below) */
    put32(shifted_logic_code + 8, 0x68C500FFu);  /* xoris r5,r6,0x00FF (patched below) */
    /* Encode RT as source and RA as destination: oris r4,r3,ABCD; xoris r5,r4,00FF. */
    put32(shifted_logic_code + 4, (25u << 26) | (3u << 21) | (4u << 16) | 0xABCDu);
    put32(shifted_logic_code + 8, (27u << 26) | (4u << 21) | (5u << 16) | 0x00FFu);
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x7000u, sizeof(shifted_logic_code),
        shifted_logic_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, 0x7000u, registers, 3u, &steps) == PPC_CPU_STEP_LIMIT);
    CHECK(steps == 3u);
    CHECK(registers[4] == 0xABCD1234u);
    CHECK(registers[5] == (0xABCD1234u ^ 0x00FF0000u));

    /* Fetching from unmapped/non-executable memory must fail safely. */
    ps3_guest_memory_init(&memory);
    steps = 99u;
    CHECK(ppc_cpu_run_guest(&memory, 0x4000u, registers, 4u, &steps) ==
        PPC_CPU_MEMORY_ACCESS_ERROR);
    CHECK(steps == 0u);

    /* End-to-end smoke test: load a tiny ELF and execute its PPC entry point. */
    memset(file, 0, sizeof(file));
    memcpy(file, "\x7f""ELF", 4);
    file[4] = 2u; file[5] = 2u; file[6] = 1u;
    put16(file + 16, 2u); put16(file + 18, 21u); put32(file + 20, 1u);
    put64(file + 24, 0x10000u); put64(file + 32, 64u);
    put16(file + 52, 64u); put16(file + 54, 56u); put16(file + 56, 1u);
    put32(file + 64, 1u); put32(file + 68, 5u);
    put64(file + 72, 120u); put64(file + 80, 0x10000u);
    put64(file + 88, 0x10000u); put64(file + 96, 16u);
    put64(file + 104, 16u); put64(file + 112, 4u);
    put32(file + 120, 0x38600007u); /* addi r3,0,7 */
    put32(file + 124, 0x38800005u); /* addi r4,0,5 */
    put32(file + 128, 0x7ca32214u); /* add r5,r3,r4 */
    put32(file + 132, 0x4e800020u); /* blr: clean top-level return in this prototype */

    memset(ram, 0, sizeof(ram));
    ps3_guest_memory_init(&memory);
    entry = 0u;
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_OK);
    CHECK(entry == 0x10000u);
    memset(registers, 0, sizeof(registers));
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, entry, registers, 4u, &steps) ==
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 4u);
    CHECK(registers[3] == 7u);
    CHECK(registers[4] == 5u);
    CHECK(registers[5] == 12u);

    ps3_guest_memory_init(&memory);
    file[31] = 0x80u; /* entry outside the executable PT_LOAD segment */
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);

    puts("Native ELF loader and entry-point execution tests passed.");
    return 0;
}
