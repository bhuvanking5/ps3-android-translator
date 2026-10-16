#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../android/app/src/main/cpp/ppc_cpu.h"

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "CHECK failed at %s:%d: %s\\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static uint32_t addi(unsigned rt, unsigned ra, int imm) {
    return (14u << 26) | (rt << 21) | (ra << 16) | ((uint32_t)imm & 0xffffu);
}
static uint32_t branch(int displacement, unsigned absolute, unsigned link) {
    return (18u << 26) | ((uint32_t)displacement & 0x03FFFFFCu) |
           ((absolute & 1u) << 1) | (link & 1u);
}
static uint32_t xo(unsigned rt, unsigned ra, unsigned rb, unsigned ext) {
    return (31u << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (ext << 1);
}

int main(void) {
    uint64_t gpr[32] = {0};
    const uint32_t program[] = {
        addi(3, 0, 7), addi(4, 3, 5), xo(5, 3, 4, 266), xo(6, 3, 5, 40)
    };
    CHECK(ppc_cpu_run(program, 4, gpr) == PPC_CPU_OK);
    CHECK(gpr[3] == 7 && gpr[4] == 12 && gpr[5] == 19 && gpr[6] == 12);

    memset(gpr, 0, sizeof(gpr));
    const uint32_t arithmetic[] = {
        addi(3, 0, 6), addi(4, 0, 7), xo(5, 3, 4, 233), xo(5, 5, 3, 444)
    };
    CHECK(ppc_cpu_run(arithmetic, 4, gpr) == PPC_CPU_OK);
    CHECK(gpr[5] == (42u | 6u));

    memset(gpr, 0, sizeof(gpr));
    const uint32_t logical[] = {
        addi(3, 0, 12), addi(4, 0, 10),
        xo(3, 5, 4, 28), xo(3, 6, 4, 316), xo(3, 7, 4, 444)
    };
    CHECK(ppc_cpu_run(logical, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[5] == 8u && gpr[6] == 6u && gpr[7] == 14u);

    memset(gpr, 0, sizeof(gpr));
    const uint32_t negative[] = { addi(3, 0, -2) };
    CHECK(ppc_cpu_run(negative, 1, gpr) == PPC_CPU_OK);
    CHECK(gpr[3] == UINT64_MAX - 1u);

    memset(gpr, 0, sizeof(gpr));
    const uint32_t branch_program[] = {
        addi(3, 0, 1), branch(8, 0, 0), addi(3, 0, 99), addi(4, 0, 7)
    };
    CHECK(ppc_cpu_run(branch_program, 4, gpr) == PPC_CPU_OK);
    CHECK(gpr[3] == 1u && gpr[4] == 7u); /* branch skips the middle ADDI */

    const uint32_t looping_branch[] = { branch(0, 0, 0) };
    CHECK(ppc_cpu_run(looping_branch, 1, gpr) == PPC_CPU_STEP_LIMIT);
    const uint32_t out_of_range_branch[] = { branch(8, 0, 0) };
    CHECK(ppc_cpu_run(out_of_range_branch, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    const uint32_t linked_branch[] = { branch(0, 0, 1) };
    CHECK(ppc_cpu_run(linked_branch, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);

    CHECK(ppc_cpu_run(program, 0, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    CHECK(ppc_cpu_run(program, PPC_CPU_MAX_INSTRUCTIONS + 1u, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    const uint32_t unsupported[] = { 0u };
    CHECK(ppc_cpu_run(unsupported, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    CHECK(ppc_cpu_run(NULL, 1, gpr) == PPC_CPU_INVALID_ARGUMENT);
    CHECK(ppc_cpu_run(program, 1, NULL) == PPC_CPU_INVALID_ARGUMENT);

    puts("PowerPC subset tests passed (arithmetic/logical ops, local branches, step limit, bounds, unsupported instructions).");
    return 0;
}
