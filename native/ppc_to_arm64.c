#include "ppc_to_arm64.h"

#include <limits.h>

#define PPC_MAX_INSTRUCTIONS 256u
#define PPC_GPR_LIMIT 18u

static int emit(uint32_t *output, size_t capacity, size_t *count,
                uint32_t instruction) {
    if (*count >= capacity) return 0;
    output[(*count)++] = instruction;
    return 1;
}

static int valid_gpr(unsigned reg) {
    return reg < PPC_GPR_LIMIT;
}

static uint32_t mov_register(unsigned rd, unsigned rm) {
    /* MOV Xd, Xm is ORR Xd, XZR, Xm. */
    return 0xAA0003E0u | (rm << 16) | rd;
}

ppc_to_arm64_status ppc_translate_to_arm64(
    const uint32_t *instructions, size_t count,
    unsigned result_gpr, uint32_t *output, size_t output_capacity,
    size_t *output_count) {
    if (output_count == 0) return PPC_TO_ARM64_INVALID_ARGUMENT;
    *output_count = 0;

    if (instructions == 0 || output == 0 || count == 0 ||
        !valid_gpr(result_gpr)) {
        return PPC_TO_ARM64_INVALID_ARGUMENT;
    }
    if (count > PPC_MAX_INSTRUCTIONS) return PPC_TO_ARM64_INSTRUCTION_LIMIT;
    if (output_capacity < count + 2u) return PPC_TO_ARM64_OUTPUT_TOO_SMALL;

    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        const uint32_t ins = instructions[i];
        const unsigned opcode = ins >> 26;
        const unsigned rt = (ins >> 21) & 31u;
        const unsigned ra = (ins >> 16) & 31u;
        const unsigned rb = (ins >> 11) & 31u;
        uint32_t arm = 0;

        if (opcode == 14u) {
            const int32_t imm = (int16_t)(ins & 0xffffu);
            if (!valid_gpr(rt) || !valid_gpr(ra))
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;

            if (ra == 0u) {
                /* ADDI with RA=0 uses literal zero, not PPC GPR0. */
                if (imm >= 0) {
                    arm = 0xD2800000u | ((uint32_t)imm << 5) | rt; /* MOVZ */
                } else {
                    const uint32_t inverse = (uint32_t)(-imm - 1);
                    arm = 0x92800000u | (inverse << 5) | rt; /* MOVN */
                }
            } else if (imm >= 0 && imm <= 4095) {
                arm = 0x91000000u | ((uint32_t)imm << 10) |
                      (ra << 5) | rt; /* ADD immediate */
            } else if (imm < 0 && imm >= -4095) {
                arm = 0xD1000000u | ((uint32_t)(-imm) << 10) |
                      (ra << 5) | rt; /* SUB immediate */
            } else {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
        } else if (opcode == 31u) {
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if ((ins & 1u) != 0u || !valid_gpr(rt) ||
                !valid_gpr(ra) || !valid_gpr(rb)) {
                /* Record-form instructions update CR, which this subset omits. */
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }

            if (xo == 266u) { /* ADD RT, RA, RB */
                arm = 0x8B000000u | (rb << 16) | (ra << 5) | rt;
            } else if (xo == 40u) { /* SUBF RT, RA, RB => RB - RA */
                arm = 0xCB000000u | (ra << 16) | (rb << 5) | rt;
            } else if (xo == 233u) { /* MULLD */
                arm = 0x9B007C00u | (rb << 16) | (ra << 5) | rt;
            } else if (xo == 444u) { /* OR RA, RT, RB */
                arm = 0xAA000000u | (rb << 16) | (rt << 5) | ra;
            } else if (xo == 28u) { /* AND RA, RT, RB */
                arm = 0x8A000000u | (rb << 16) | (rt << 5) | ra;
            } else if (xo == 316u) { /* XOR RA, RT, RB */
                arm = 0xCA000000u | (rb << 16) | (rt << 5) | ra;
            } else {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
        } else {
            return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
        }

        if (!emit(output, output_capacity, &used, arm))
            return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
    }

    if (!emit(output, output_capacity, &used, mov_register(0u, result_gpr)) ||
        !emit(output, output_capacity, &used, 0xD65F03C0u)) {
        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
    }
    *output_count = used;
    return PPC_TO_ARM64_OK;
}
