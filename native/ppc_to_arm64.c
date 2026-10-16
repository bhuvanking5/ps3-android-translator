#include "ppc_to_arm64.h"

#define PPC_MAX_INSTRUCTIONS 256u
#define PPC_GPR_COUNT 32u
#define PPC_FRAME_BYTES 320u
#define SCRATCH_A 16u
#define SCRATCH_B 17u

static int emit(uint32_t *output, size_t capacity, size_t *count,
                uint32_t instruction) {
    if (*count >= capacity) return 0;
    output[(*count)++] = instruction;
    return 1;
}

static uint32_t movz(unsigned rd, unsigned imm16) {
    return 0xD2800000u | ((uint32_t)(imm16 & 0xffffu) << 5) | rd;
}

static uint32_t movn(unsigned rd, unsigned imm16) {
    return 0x92800000u | ((uint32_t)(imm16 & 0xffffu) << 5) | rd;
}

static uint32_t load_gpr(unsigned target, unsigned ppc_reg) {
    /* LDR Xt, [SP, #(ppc_reg * 8)] */
    return 0xF9400000u | (ppc_reg << 10) | (31u << 5) | target;
}

static uint32_t store_gpr(unsigned source, unsigned ppc_reg) {
    /* STR Xt, [SP, #(ppc_reg * 8)] */
    return 0xF9000000u | (ppc_reg << 10) | (31u << 5) | source;
}

static uint32_t add_reg(unsigned rd, unsigned rn, unsigned rm) {
    return 0x8B000000u | (rm << 16) | (rn << 5) | rd;
}

static uint32_t sub_reg(unsigned rd, unsigned rn, unsigned rm) {
    return 0xCB000000u | (rm << 16) | (rn << 5) | rd;
}

static uint32_t mul_reg(unsigned rd, unsigned rn, unsigned rm) {
    return 0x9B007C00u | (rm << 16) | (rn << 5) | rd;
}

static uint32_t logical_reg(unsigned base, unsigned rd,
                            unsigned rn, unsigned rm) {
    return base | (rm << 16) | (rn << 5) | rd;
}

static int emit_gpr_load(uint32_t *out, size_t cap, size_t *used,
                         unsigned scratch, unsigned ppc_reg) {
    return emit(out, cap, used, load_gpr(scratch, ppc_reg));
}

static int emit_gpr_store(uint32_t *out, size_t cap, size_t *used,
                          unsigned scratch, unsigned ppc_reg) {
    return emit(out, cap, used, store_gpr(scratch, ppc_reg));
}

ppc_to_arm64_status ppc_translate_to_arm64(
    const uint32_t *instructions, size_t count,
    unsigned result_gpr, uint32_t *output, size_t output_capacity,
    size_t *output_count) {
    if (output_count == 0) return PPC_TO_ARM64_INVALID_ARGUMENT;
    *output_count = 0;

    if (instructions == 0 || output == 0 || count == 0 ||
        result_gpr >= PPC_GPR_COUNT) {
        return PPC_TO_ARM64_INVALID_ARGUMENT;
    }
    if (count > PPC_MAX_INSTRUCTIONS) return PPC_TO_ARM64_INSTRUCTION_LIMIT;

    size_t used = 0;
    size_t instruction_output_offsets[PPC_MAX_INSTRUCTIONS] = {0};
    size_t branch_word_positions[PPC_MAX_INSTRUCTIONS] = {0};
    size_t branch_target_indices[PPC_MAX_INSTRUCTIONS] = {0};
    unsigned char is_branch[PPC_MAX_INSTRUCTIONS] = {0};
    /* Frame stores 32 GPRs plus 32 individual CR bits in 64-bit slots. */
    if (!emit(output, output_capacity, &used,
              0xD1000000u | (PPC_FRAME_BYTES << 10) | (31u << 5) | 31u)) {
        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
    }

    /* The reference interpreter starts every GPR at zero. Initialize slots. */
    if (!emit(output, output_capacity, &used, movz(SCRATCH_A, 0u))) {
        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
    }
    for (unsigned reg = 0; reg < 40u; ++reg) {
        if (!emit(output, output_capacity, &used, store_gpr(SCRATCH_A, reg))) {
            return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
        }
    }

    for (size_t i = 0; i < count; ++i) {
        instruction_output_offsets[i] = used;
        const uint32_t ins = instructions[i];
        const unsigned opcode = ins >> 26;
        const unsigned rt = (ins >> 21) & 31u;
        const unsigned ra = (ins >> 16) & 31u;
        const unsigned rb = (ins >> 11) & 31u;

        if (opcode == 16u) {
            /* bc subset: local relative branch on one CR bit (BO=4/12). */
            const unsigned bo = (ins >> 21) & 31u;
            const unsigned bi = (ins >> 16) & 31u;
            if ((ins & 3u) != 0u || (bo != 4u && bo != 12u)) {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
            int32_t displacement = (int32_t)(ins & 0x0000FFFCu);
            if ((displacement & 0x00008000) != 0) {
                displacement |= (int32_t)0xFFFF0000u;
            }
            const int64_t target_index = (int64_t)i + displacement / 4;
            if (target_index < 0 || target_index >= (int64_t)count) {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
            if (!emit_gpr_load(output, output_capacity, &used,
                               SCRATCH_A, 32u + bi)) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            branch_word_positions[i] = used;
            branch_target_indices[i] = (size_t)target_index;
            is_branch[i] = (bo == 12u) ? 2u : 3u;
            if (!emit(output, output_capacity, &used,
                      (bo == 12u ? 0xB5000000u : 0xB4000000u) | SCRATCH_A)) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            continue;
        }

        if (opcode == 11u) {
            /* cmpwi: compare signed low 32 bits; L=1 (cmpdi) is unsupported. */
            const unsigned bf = (ins >> 23) & 7u;
            const unsigned cmp_ra = (ins >> 16) & 31u;
            const int32_t imm = (int16_t)(ins & 0xffffu);
            if (((ins >> 21) & 1u) != 0u) {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
            if (!emit_gpr_load(output, output_capacity, &used,
                               SCRATCH_A, cmp_ra) ||
                !emit(output, output_capacity, &used,
                      imm >= 0 ? movz(SCRATCH_B, (unsigned)imm)
                               : movn(SCRATCH_B, (unsigned)(-imm - 1))) ||
                !emit(output, output_capacity, &used,
                      0x6B00001Fu | (SCRATCH_B << 16) |
                      (SCRATCH_A << 5))) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            const unsigned cr_base = 32u + 4u * bf;
            const unsigned conds[3] = {11u, 12u, 0u}; /* LT, GT, EQ */
            for (unsigned bit = 0; bit < 3u; ++bit) {
                const uint32_t cset =
                    0x1A9F07E0u | ((conds[bit] ^ 1u) << 12) | SCRATCH_B;
                if (!emit(output, output_capacity, &used, cset) ||
                    !emit_gpr_store(output, output_capacity, &used,
                                    SCRATCH_B, cr_base + bit)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
            }
            /* SO is not modelled by this subset, so write it as zero. */
            if (!emit(output, output_capacity, &used, movz(SCRATCH_B, 0u)) ||
                !emit_gpr_store(output, output_capacity, &used,
                                SCRATCH_B, cr_base + 3u)) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            continue;
        }

        if (opcode == 18u) {
            /* Only local relative b is supported: no AA or LK. */
            if ((ins & 3u) != 0u) return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            int32_t displacement = (int32_t)(ins & 0x03FFFFFCu);
            if ((displacement & 0x02000000) != 0) {
                displacement |= (int32_t)0xFC000000u;
            }
            if ((displacement & 3) != 0) return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            const int64_t target_index = (int64_t)i + displacement / 4;
            if (target_index < 0 || target_index >= (int64_t)count) {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
            branch_word_positions[i] = used;
            branch_target_indices[i] = (size_t)target_index;
            is_branch[i] = 1u;
            if (!emit(output, output_capacity, &used, 0x14000000u)) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            continue;
        }

        if (opcode == 14u) {
            const int32_t imm = (int16_t)(ins & 0xffffu);
            if (ra == 0u) {
                /* ADDI treats RA=0 as literal zero, not GPR r0. */
                if (imm >= 0) {
                    if (!emit(output, output_capacity, &used,
                              movz(SCRATCH_A, (unsigned)imm))) {
                        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                    }
                } else if (!emit(output, output_capacity, &used,
                                 movn(SCRATCH_A, (unsigned)(-imm - 1)))) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
            } else {
                if (!emit_gpr_load(output, output_capacity, &used,
                                   SCRATCH_A, ra)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
                if (imm > 0 && imm <= 4095) {
                    if (!emit(output, output_capacity, &used,
                              0x91000000u | ((uint32_t)imm << 10) |
                              (SCRATCH_A << 5) | SCRATCH_A)) {
                        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                    }
                } else if (imm < 0 && imm >= -4095) {
                    if (!emit(output, output_capacity, &used,
                              0xD1000000u | ((uint32_t)(-imm) << 10) |
                              (SCRATCH_A << 5) | SCRATCH_A)) {
                        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                    }
                } else if (imm != 0) {
                    uint32_t constant;
                    if (imm > 0) {
                        constant = movz(SCRATCH_B, (unsigned)imm);
                    } else {
                        constant = movz(SCRATCH_B, (unsigned)(-imm));
                    }
                    if (!emit(output, output_capacity, &used, constant) ||
                        !emit(output, output_capacity, &used,
                              imm > 0
                                  ? add_reg(SCRATCH_A, SCRATCH_A, SCRATCH_B)
                                  : sub_reg(SCRATCH_A, SCRATCH_A, SCRATCH_B))) {
                        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                    }
                }
            }
            if (!emit_gpr_store(output, output_capacity, &used, SCRATCH_A, rt)) {
                return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
            }
            continue;
        }

        if (opcode == 31u) {
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if ((ins & 1u) != 0u) {
                /* Record forms update the condition register, not implemented. */
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }

            if (xo == 266u || xo == 40u || xo == 233u) {
                if (!emit_gpr_load(output, output_capacity, &used,
                                   SCRATCH_A, ra) ||
                    !emit_gpr_load(output, output_capacity, &used,
                                   SCRATCH_B, rb)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
                uint32_t arm;
                if (xo == 266u) arm = add_reg(SCRATCH_A, SCRATCH_A, SCRATCH_B);
                else if (xo == 40u) arm = sub_reg(SCRATCH_A, SCRATCH_B, SCRATCH_A);
                else arm = mul_reg(SCRATCH_A, SCRATCH_A, SCRATCH_B);
                if (!emit(output, output_capacity, &used, arm) ||
                    !emit_gpr_store(output, output_capacity, &used, SCRATCH_A, rt)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
            } else if (xo == 444u || xo == 28u || xo == 316u) {
                /* PowerPC logical forms write RA and read RT plus RB. */
                if (!emit_gpr_load(output, output_capacity, &used,
                                   SCRATCH_A, rt) ||
                    !emit_gpr_load(output, output_capacity, &used,
                                   SCRATCH_B, rb)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
                const uint32_t base = xo == 444u ? 0xAA000000u
                                      : xo == 28u ? 0x8A000000u
                                                  : 0xCA000000u;
                if (!emit(output, output_capacity, &used,
                          logical_reg(base, SCRATCH_A, SCRATCH_A, SCRATCH_B)) ||
                    !emit_gpr_store(output, output_capacity, &used, SCRATCH_A, ra)) {
                    return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
                }
            } else {
                return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
            }
            continue;
        }

        return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
    }

    /* Resolve local B and CBZ/CBNZ offsets after all target offsets are known. */
    for (size_t i = 0; i < count; ++i) {
        if (is_branch[i]) {
            const int64_t delta_words =
                (int64_t)instruction_output_offsets[branch_target_indices[i]] -
                (int64_t)branch_word_positions[i];
            if (is_branch[i] == 1u) {
                if (delta_words < -(1LL << 25) || delta_words >= (1LL << 25)) {
                    return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
                }
                output[branch_word_positions[i]] =
                    0x14000000u | ((uint32_t)delta_words & 0x03FFFFFFu);
            } else {
                if (delta_words < -(1LL << 18) || delta_words >= (1LL << 18)) {
                    return PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION;
                }
                output[branch_word_positions[i]] =
                    (output[branch_word_positions[i]] & 0xFF00001Fu) |
                    (((uint32_t)delta_words & 0x7FFFFu) << 5);
            }
        }
    }

    if (!emit(output, output_capacity, &used, load_gpr(0u, result_gpr)) ||
        !emit(output, output_capacity, &used,
              0x91000000u | (PPC_FRAME_BYTES << 10) | (31u << 5) | 31u) ||
        !emit(output, output_capacity, &used, 0xD65F03C0u)) {
        return PPC_TO_ARM64_OUTPUT_TOO_SMALL;
    }

    *output_count = used;
    return PPC_TO_ARM64_OK;
}
