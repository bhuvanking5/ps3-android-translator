#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../android/app/src/main/cpp/ppc_cpu.h"

static uint32_t addi(unsigned rt, unsigned ra, int imm) {
    return (14u << 26) | (rt << 21) | (ra << 16) | ((uint32_t)imm & 0xffffu);
}
static uint32_t xo(unsigned rt, unsigned ra, unsigned rb, unsigned ext) {
    return (31u << 26) | (rt << 21) | (ra << 16) | (rb << 11) | (ext << 1);
}

int main(void) {
    uint64_t gpr[32] = {0};
    const uint32_t program[] = {
        addi(3, 0, 7), addi(4, 3, 5), xo(5, 3, 4, 266), xo(6, 3, 5, 40)
    };
    assert(ppc_cpu_run(program, 4, gpr) == PPC_CPU_OK);
    assert(gpr[3] == 7 && gpr[4] == 12 && gpr[5] == 19 && gpr[6] == 12);

    memset(gpr, 0, sizeof(gpr));
    const uint32_t arithmetic[] = {
        addi(3, 0, 6), addi(4, 0, 7), xo(5, 3, 4, 233), xo(5, 5, 3, 444)
    };
    assert(ppc_cpu_run(arithmetic, 4, gpr) == PPC_CPU_OK);
    assert(gpr[5] == (42u | 6u));

    memset(gpr, 0, sizeof(gpr));
    const uint32_t negative[] = { addi(3, 0, -2) };
    assert(ppc_cpu_run(negative, 1, gpr) == PPC_CPU_OK);
    assert(gpr[3] == UINT64_MAX - 1u);

    assert(ppc_cpu_run(program, 0, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    assert(ppc_cpu_run(program, PPC_CPU_MAX_INSTRUCTIONS + 1u, gpr) == PPC_CPU_INSTRUCTION_LIMIT);
    const uint32_t unsupported[] = { 0u };
    assert(ppc_cpu_run(unsupported, 1, gpr) == PPC_CPU_UNSUPPORTED_INSTRUCTION);
    assert(ppc_cpu_run(NULL, 1, gpr) == PPC_CPU_INVALID_ARGUMENT);
    assert(ppc_cpu_run(program, 1, NULL) == PPC_CPU_INVALID_ARGUMENT);

    puts("PowerPC subset tests passed (ADDI/ADD/SUBF, negative immediate, bounds, unsupported opcode).");
    return 0;
}
