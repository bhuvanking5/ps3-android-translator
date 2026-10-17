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

static int allow_foo_import(const char *name, uint64_t *value, void *context) {
    int *calls = (int *)context;
    ++*calls;
    if (strcmp(name, "foo") != 0) return 0;
    *value = 0x1010u;
    return 1;
}

static int allow_foo_too_high(const char *name, uint64_t *value, void *context) {
    int *calls = (int *)context;
    ++*calls;
    if (strcmp(name, "foo") != 0) return 0;
    *value = (uint64_t)UINT32_MAX + 1u;
    return 1;
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

    /* A linked subroutine call must return through LR and preserve the caller LR. */
    uint8_t call_code[24] = {0};
    put32(call_code, 0x7c0802a6u);      /* mflr r0: save top-level return sentinel */
    put32(call_code + 4, 0x4800000du);  /* bl +12: call subroutine at 0x8010 */
    put32(call_code + 8, 0x7c0803a6u);  /* mtlr r0: restore caller LR */
    put32(call_code + 12, 0x4e800020u); /* blr: exit top-level guest */
    put32(call_code + 16, 0x3860002au); /* addi r3,0,42 */
    put32(call_code + 20, 0x4e800020u); /* blr: return to caller */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x8000u, sizeof(call_code), call_code,
        PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, 0x8000u, registers, 8u, &steps) ==
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 6u);
    CHECK(registers[3] == 42u);

    /* CTR-based indirect call/return path used by common PowerPC dispatch stubs. */
    uint8_t ctr_call_code[28] = {0};
    put32(ctr_call_code, 0x7c0802a6u);      /* mflr r0 */
    put32(ctr_call_code + 4, 0x7ca903a6u);  /* mtctr r5 */
    put32(ctr_call_code + 8, 0x4e800421u);  /* bctrl: call address in CTR */
    put32(ctr_call_code + 12, 0x7c0803a6u); /* mtlr r0 */
    put32(ctr_call_code + 16, 0x4e800020u); /* blr: top-level exit */
    put32(ctr_call_code + 20, 0x3860002au); /* addi r3,0,42 */
    put32(ctr_call_code + 24, 0x4e800020u); /* blr: return to bctrl caller */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x9000u, sizeof(ctr_call_code),
        ctr_call_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    registers[5] = 0x9014u;
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, 0x9000u, registers, 8u, &steps) ==
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 7u);
    CHECK(registers[3] == 42u);

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

    /* The actual guest-memory runner must support the newly tested rotate/shift subset. */
    uint8_t rotate_shift_code[32] = {0};
    put32(rotate_shift_code + 0, 0x38601234u); /* addi r3,0,0x1234 */
    put32(rotate_shift_code + 4, 0x38800008u); /* addi r4,0,8 */
    put32(rotate_shift_code + 8, (21u << 26) | (3u << 21) | (5u << 16) |
        (8u << 11) | (31u << 1)); /* rlwinm r5,r3,8,0,31 */
    put32(rotate_shift_code + 12, (23u << 26) | (3u << 21) | (6u << 16) |
        (4u << 11) | (31u << 1)); /* rlwnm r6,r3,r4,0,31 */
    put32(rotate_shift_code + 16, 0x38E00004u); /* addi r7,0,4 */
    put32(rotate_shift_code + 20, (31u << 26) | (3u << 21) | (8u << 16) |
        (7u << 11) | (24u << 1)); /* slw r8,r3,r7 */
    put32(rotate_shift_code + 24, (31u << 26) | (3u << 21) | (9u << 16) |
        (7u << 11) | (536u << 1)); /* srw r9,r3,r7 */
    put32(rotate_shift_code + 28, 0x4e800020u); /* blr */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x6000u, sizeof(rotate_shift_code),
        rotate_shift_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, 0x6000u, registers, 8u, &steps) ==
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 8u);
    CHECK(registers[5] == 0x00123400u);
    CHECK(registers[6] == 0x00123400u);
    CHECK(registers[8] == 0x00012340u);
    CHECK(registers[9] == 0x00000123u);

    /* ANDI. in the actual guest runner must update CR0 for a following branch. */
    uint8_t andi_branch_code[24] = {0};
    put32(andi_branch_code + 0, 0x38600008u); /* addi r3,0,8 */
    put32(andi_branch_code + 4, (28u << 26) | (3u << 21) | (4u << 16) | 3u); /* andi. r4,r3,3 => zero */
    put32(andi_branch_code + 8, 0x41820008u); /* beq cr0,+8 */
    put32(andi_branch_code + 12, 0x38A00063u); /* addi r5,0,99 (must be skipped) */
    put32(andi_branch_code + 16, 0x38A0000Cu); /* addi r5,0,12 */
    put32(andi_branch_code + 20, 0x4e800020u); /* blr */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x5000u, sizeof(andi_branch_code),
        andi_branch_code, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_EXECUTE));
    memset(registers, 0, sizeof(registers));
    steps = 0u;
    CHECK(ppc_cpu_run_guest(&memory, 0x5000u, registers, 8u, &steps) ==
        PPC_CPU_GUEST_EXIT);
    CHECK(steps == 5u && registers[4] == 0u && registers[5] == 12u);

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

    /* PowerPC ELF entry points must be instruction-aligned. */
    file[31] = 0x02u; /* entry = 0x10002, inside the segment but misaligned */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);
    file[31] = 0u; /* restore aligned entry 0x10000 */

    /* Reject a truncated program-header table without mapping guest memory. */
    file[31] = 0u; /* restore entry to 0x10000 */
    put64(file + 32, 140u); /* one 56-byte header cannot fit from offset 140 */
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);

    /* Reject file-backed bytes larger than the segment's memory size. */
    put64(file + 32, 64u);
    put64(file + 96, 17u);
    put64(file + 104, 16u);
    memset(ram, 0xa5, sizeof(ram));
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_BAD_FORMAT);
    CHECK(memory.count == 0u);
    CHECK(ram[0] == 0xa5u && ram[15] == 0xa5u);

    /* Reject a segment that extends one byte past the guest RAM window. */
    put64(file + 96, 16u);
    put64(file + 80, 0x10031u);
    put64(file + 88, 0x10031u);
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_OUT_OF_BOUNDS);
    CHECK(memory.count == 0u);

    /* A pre-existing mapping collision must be detected before RAM is changed. */
    put64(file + 80, 0x10000u);
    put64(file + 88, 0x10000u);
    uint8_t occupied[16] = {0};
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_memory_map(&memory, 0x10000u, sizeof(occupied), occupied,
        PS3_GUEST_MEMORY_READ));
    memset(ram, 0xa5, sizeof(ram));
    CHECK(ps3_guest_elf_load(file, sizeof(file), &memory, ram, 0x10000u,
        sizeof(ram), &entry) == PS3_GUEST_ELF_MEMORY_MAP_ERROR);
    CHECK(memory.count == 1u);
    CHECK(ram[0] == 0xa5u && ram[15] == 0xa5u);


    /* Dynamic ELF must not enter the current native runner before relocations
       and imports are supported. Reject it before changing guest RAM or maps. */
    uint8_t dynamic_file[208] = {0};
    memcpy(dynamic_file, "\x7f""ELF", 4);
    dynamic_file[4] = 2u; dynamic_file[5] = 2u; dynamic_file[6] = 1u;
    put16(dynamic_file + 16, 2u); put16(dynamic_file + 18, 21u);
    put32(dynamic_file + 20, 1u);
    put64(dynamic_file + 24, 0x10000u); put64(dynamic_file + 32, 64u);
    put16(dynamic_file + 52, 64u); put16(dynamic_file + 54, 56u);
    put16(dynamic_file + 56, 2u);
    put32(dynamic_file + 64, 2u); /* PT_DYNAMIC appears first deliberately. */
    put64(dynamic_file + 72, 192u); put64(dynamic_file + 80, 0x10010u);
    put64(dynamic_file + 96, 16u); put64(dynamic_file + 104, 16u);
    put32(dynamic_file + 120, 1u); put32(dynamic_file + 124, 5u);
    put64(dynamic_file + 128, 176u); put64(dynamic_file + 136, 0x10000u);
    put64(dynamic_file + 144, 0x10000u); put64(dynamic_file + 152, 16u);
    put64(dynamic_file + 160, 16u); put64(dynamic_file + 168, 4u);
    put32(dynamic_file + 176, 0x38600007u);
    put32(dynamic_file + 180, 0x38800005u);
    put32(dynamic_file + 184, 0x7ca32214u);
    put32(dynamic_file + 188, 0x4e800020u);
    put64(dynamic_file + 192, 7u); /* malformed dynamic table: no DT_NULL */
    put64(dynamic_file + 200, 0x10000u);
    memset(ram, 0xa5, sizeof(ram));
    ps3_guest_memory_init(&memory);
    ps3_guest_elf_status dynamic_status = ps3_guest_elf_load(dynamic_file,
        sizeof(dynamic_file), &memory, ram, 0x10000u, sizeof(ram), &entry);
    CHECK(dynamic_status == PS3_GUEST_ELF_RELOCATION_REQUIRED);
    CHECK(memory.count == 0u);
    CHECK(ram[0] == 0xa5u && ram[15] == 0xa5u);

    /* ET_DYN also requires load-bias/relocation support even without PT_DYNAMIC. */
    put16(dynamic_file + 16, 3u);
    put16(dynamic_file + 56, 1u);
    memset(ram, 0xa5, sizeof(ram));
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(dynamic_file, sizeof(dynamic_file), &memory,
        ram, 0x10000u, sizeof(ram), &entry) ==
        PS3_GUEST_ELF_RELOCATION_REQUIRED);
    CHECK(memory.count == 0u);
    CHECK(ram[0] == 0xa5u && ram[15] == 0xa5u);

    /* End-to-end restricted dynamic RELA load: parse, resolve, stage, relocate, commit. */
    uint8_t dynamic_elf[512] = {0};
    uint8_t dynamic_ram[512];
    int import_calls = 0;
    memcpy(dynamic_elf, "\x7f" "ELF", 4u);
    dynamic_elf[4] = 2u; dynamic_elf[5] = 2u; dynamic_elf[6] = 1u;
    put16(dynamic_elf + 16, 2u); put16(dynamic_elf + 18, 21u);
    put32(dynamic_elf + 20, 1u);
    put64(dynamic_elf + 24, 0x1000u); put64(dynamic_elf + 32, 64u);
    put16(dynamic_elf + 52, 64u); put16(dynamic_elf + 54, 56u);
    put16(dynamic_elf + 56, 3u);
    /* Executable PT_LOAD: file offset 232 -> guest address 0x1000. */
    put32(dynamic_elf + 64, 1u); put32(dynamic_elf + 68, 5u);
    put64(dynamic_elf + 72, 232u); put64(dynamic_elf + 80, 0x1000u);
    put64(dynamic_elf + 88, 0x1000u); put64(dynamic_elf + 96, 16u);
    put64(dynamic_elf + 104, 16u); put64(dynamic_elf + 112, 4u);
    /* Writable PT_LOAD: dynamic metadata, RELA, symbol and string tables. */
    put32(dynamic_elf + 120, 1u); put32(dynamic_elf + 124, 6u);
    put64(dynamic_elf + 128, 248u); put64(dynamic_elf + 136, 0x1080u);
    put64(dynamic_elf + 144, 0x1080u); put64(dynamic_elf + 152, 264u);
    put64(dynamic_elf + 160, 264u); put64(dynamic_elf + 168, 8u);
    /* PT_DYNAMIC is intentionally the third header. */
    put32(dynamic_elf + 176, 2u);
    put64(dynamic_elf + 184, 248u); put64(dynamic_elf + 192, 0x1080u);
    put64(dynamic_elf + 208, 128u); put64(dynamic_elf + 216, 128u);
    /* DT_RELA, DT_RELASZ, DT_RELAENT, DT_SYMTAB, DT_SYMENT, DT_STRTAB, DT_STRSZ, DT_NULL. */
    put64(dynamic_elf + 248, 7u); put64(dynamic_elf + 256, 0x1100u);
    put64(dynamic_elf + 264, 8u); put64(dynamic_elf + 272, 24u);
    put64(dynamic_elf + 280, 9u); put64(dynamic_elf + 288, 24u);
    put64(dynamic_elf + 296, 6u); put64(dynamic_elf + 304, 0x1120u);
    put64(dynamic_elf + 312, 11u); put64(dynamic_elf + 320, 24u);
    put64(dynamic_elf + 328, 5u); put64(dynamic_elf + 336, 0x1160u);
    put64(dynamic_elf + 344, 10u); put64(dynamic_elf + 352, 16u);
    put64(dynamic_elf + 360, 0u); put64(dynamic_elf + 368, 0u);
    /* RELA: target 0x10a0, symbol index 1, R_PPC64_ADDR32, addend 4. */
    put64(dynamic_elf + 376, 0x10a0u);
    put64(dynamic_elf + 384, (1ull << 32) | 1u);
    put64(dynamic_elf + 392, 4u);
    /* Symbol zero at 0x1120; symbol one at 0x1138, defined value 0x1004. */
    put32(dynamic_elf + 432, 1u); put16(dynamic_elf + 438, 1u);
    put64(dynamic_elf + 440, 0x1004u);
    dynamic_elf[472] = 0u; memcpy(dynamic_elf + 473, "foo", 4u);
    put32(dynamic_elf + 232, 0x4e800020u); /* guest code: blr */

    memset(dynamic_ram, 0xa5, sizeof(dynamic_ram));
    ps3_guest_memory_init(&memory);
    entry = 0u;
    CHECK(ps3_guest_elf_load_with_resolver(dynamic_elf, sizeof(dynamic_elf),
        &memory, dynamic_ram, 0x1000u, sizeof(dynamic_ram), &entry,
        NULL, NULL) == PS3_GUEST_ELF_OK);
    CHECK(entry == 0x1000u && memory.count == 2u);
    CHECK(dynamic_ram[0xa0] == 0x00u && dynamic_ram[0xa1] == 0x00u &&
          dynamic_ram[0xa2] == 0x10u && dynamic_ram[0xa3] == 0x08u);

    /* Undefined imports fail by default and must leave both RAM and maps untouched. */
    put16(dynamic_elf + 438, 0u);
    memset(dynamic_ram, 0xa5, sizeof(dynamic_ram));
    ps3_guest_memory_init(&memory);
    CHECK(ps3_guest_elf_load(dynamic_elf, sizeof(dynamic_elf), &memory,
        dynamic_ram, 0x1000u, sizeof(dynamic_ram), &entry) ==
        PS3_GUEST_ELF_RELOCATION_REQUIRED);
    CHECK(memory.count == 0u && dynamic_ram[0] == 0xa5u &&
          dynamic_ram[0xa0] == 0xa5u && dynamic_ram[511] == 0xa5u);

    /* Explicitly approved imports load and relocate; out-of-range resolver results fail atomically. */
    import_calls = 0;
    CHECK(ps3_guest_elf_load_with_resolver(dynamic_elf, sizeof(dynamic_elf),
        &memory, dynamic_ram, 0x1000u, sizeof(dynamic_ram), &entry,
        allow_foo_import, &import_calls) == PS3_GUEST_ELF_OK);
    CHECK(import_calls == 1 && memory.count == 2u);
    CHECK(dynamic_ram[0xa0] == 0x00u && dynamic_ram[0xa1] == 0x00u &&
          dynamic_ram[0xa2] == 0x10u && dynamic_ram[0xa3] == 0x14u);

    memset(dynamic_ram, 0xa5, sizeof(dynamic_ram));
    ps3_guest_memory_init(&memory);
    import_calls = 0;
    CHECK(ps3_guest_elf_load_with_resolver(dynamic_elf, sizeof(dynamic_elf),
        &memory, dynamic_ram, 0x1000u, sizeof(dynamic_ram), &entry,
        allow_foo_too_high, &import_calls) == PS3_GUEST_ELF_RELOCATION_REQUIRED);
    CHECK(import_calls == 1 && memory.count == 0u &&
          dynamic_ram[0] == 0xa5u && dynamic_ram[0xa0] == 0xa5u &&
          dynamic_ram[511] == 0xa5u);

    puts("Native ELF loader and entry-point execution tests passed.");
    return 0;
}
