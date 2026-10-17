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
static uint32_t andi(unsigned rs, unsigned ra, unsigned imm) {
    return (28u << 26) | (rs << 21) | (ra << 16) | (imm & 0xffffu);
}
static uint32_t cmpwi(unsigned bf, unsigned ra, int imm) {
    return (11u << 26) | (bf << 23) | (ra << 16) |
           ((uint32_t)imm & 0xffffu);
}
static uint32_t cond_branch(unsigned bo, unsigned bi, int displacement) {
    return (16u << 26) | (bo << 21) | (bi << 16) |
           ((uint32_t)displacement & 0xfffcu);
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
    const uint32_t mulli_program[] = {
        addi(0, 0, 4), /* GPR0 can be written by ADDI */
        (7u << 26) | (4u << 21) | (0u << 16) | 3u, /* mulli r4,r0,3 */
        (7u << 26) | (5u << 21) | (4u << 16) | ((uint32_t)-2 & 0xffffu)
    };
    CHECK(ppc_cpu_run(mulli_program, 3, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 12u && gpr[5] == UINT64_MAX - 23u);

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

    memset(gpr, 0, sizeof(gpr));
    const uint32_t conditional_taken[] = {
        addi(3, 0, 7), cmpwi(0, 3, 7),
        cond_branch(12, 2, 8), addi(4, 0, 99), addi(4, 0, 1)
    };
    CHECK(ppc_cpu_run(conditional_taken, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 1u); /* EQ bit true: branch skips the unwanted write */

    memset(gpr, 0, sizeof(gpr));
    const uint32_t conditional_not_taken[] = {
        addi(3, 0, 7), cmpwi(0, 3, 8),
        cond_branch(12, 2, 8), addi(4, 0, 5), addi(4, 0, 6)
    };
    CHECK(ppc_cpu_run(conditional_not_taken, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 6u); /* EQ bit false: execution falls through */

    memset(gpr, 0, sizeof(gpr));
    const uint32_t branch_if_false[] = {
        addi(3, 0, 7), cmpwi(0, 3, 8),
        cond_branch(4, 2, 8), addi(4, 0, 99), addi(4, 0, 6)
    };
    CHECK(ppc_cpu_run(branch_if_false, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 6u); /* BO=4 branches when the selected CR bit is false */

    /* ANDI sets CR0 EQ when its masked result is zero; BC consumes that bit. */
    memset(gpr, 0, sizeof(gpr));
    const uint32_t andi_zero_branch[] = {
        addi(3, 0, 8), andi(3, 4, 3),
        cond_branch(12, 2, 8), addi(5, 0, 99), addi(5, 0, 12)
    };
    CHECK(ppc_cpu_run(andi_zero_branch, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 0u && gpr[5] == 12u);

    memset(gpr, 0, sizeof(gpr));
    const uint32_t andi_nonzero_branch[] = {
        addi(3, 0, 7), andi(3, 4, 3),
        cond_branch(12, 2, 8), addi(5, 0, 12), addi(5, 0, 99)
    };
    CHECK(ppc_cpu_run(andi_nonzero_branch, 5, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 3u && gpr[5] == 12u);

    const uint32_t unsupported_conditional[] = {
        cond_branch(16, 2, 4), addi(3, 0, 1)
    };
    CHECK(ppc_cpu_run(unsupported_conditional, 2, gpr) ==
          PPC_CPU_UNSUPPORTED_INSTRUCTION);

    const uint32_t looping_branch[] = { branch(0, 0, 0) };
    CHECK(ppc_cpu_run(looping_branch, 1, gpr) == PPC_CPU_STEP_LIMIT);
    const uint32_t out_of_range_branch[] = { branch(8, 0, 0) };
    CHECK(ppc_cpu_run(out_of_range_branch, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    const uint32_t linked_branch[] = { branch(0, 0, 1) };
    CHECK(ppc_cpu_run(linked_branch, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    const uint32_t absolute_branch[] = { branch(0, 1, 0) };
    CHECK(ppc_cpu_run(absolute_branch, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);

    /* An untaken conditional branch must not fail just because its unused
       target lies outside the supplied instruction array. */
    memset(gpr, 0, sizeof(gpr));
    const uint32_t untaken_out_of_range[] = {
        addi(3, 0, 7), cmpwi(0, 3, 8),
        cond_branch(12, 2, 0x100), addi(4, 0, 9)
    };
    CHECK(ppc_cpu_run(untaken_out_of_range, 4, gpr) == PPC_CPU_OK);
    CHECK(gpr[4] == 9u);

    const uint32_t absolute_cond_branch[] = {
        (16u << 26) | (12u << 21) | (2u << 16) | 2u
    };
    CHECK(ppc_cpu_run(absolute_cond_branch, 1, gpr) ==
          PPC_CPU_UNSUPPORTED_INSTRUCTION);

    CHECK(ppc_cpu_run(program, 0, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    CHECK(ppc_cpu_run(program, PPC_CPU_MAX_INSTRUCTIONS + 1u, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    /* Record-form X instructions require CR updates that are not implemented. */
    const uint32_t record_add[] = { xo(5, 3, 4, 266) | 1u };
    CHECK(ppc_cpu_run(record_add, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    const uint32_t record_or[] = { xo(5, 3, 4, 444) | 1u };
    CHECK(ppc_cpu_run(record_or, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);

    const uint32_t unsupported[] = { 0u };
    CHECK(ppc_cpu_run(unsupported, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    CHECK(ppc_cpu_run(NULL, 1, gpr) == PPC_CPU_INVALID_ARGUMENT);
    CHECK(ppc_cpu_run(program, 1, NULL) == PPC_CPU_INVALID_ARGUMENT);

    puts("PowerPC subset tests passed (arithmetic/logical ops, local branches, cmpwi/bc, step limit, bounds, unsupported instructions).");
    return 0;
}
