#include "ppc_cpu.h"

ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]) {
    if (instructions == 0 || registers == 0) return PPC_CPU_INVALID_ARGUMENT;
    if (count == 0u || count > PPC_CPU_MAX_INSTRUCTIONS)
        return PPC_CPU_INSTRUCTION_LIMIT;

    size_t pc = 0;
    size_t steps = 0;
    uint8_t condition[8] = {0};
    while (pc < count) {
        if (++steps > PPC_CPU_MAX_STEPS) return PPC_CPU_STEP_LIMIT;
        const uint32_t ins = instructions[pc];
        const unsigned opcode = ins >> 26;
        const unsigned rt = (ins >> 21) & 31u;
        const unsigned ra = (ins >> 16) & 31u;
        const unsigned rb = (ins >> 11) & 31u;

        if (opcode == 18u) { /* b: relative, unlinked local branch only */
            if ((ins & 3u) != 0u) return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            int32_t displacement = (int32_t)(ins & 0x03FFFFFCu);
            if ((displacement & 0x02000000) != 0) {
                displacement |= (int32_t)0xFC000000u;
            }
            const int64_t target = (int64_t)pc + displacement / 4;
            if (target < 0 || target >= (int64_t)count) {
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            pc = (size_t)target;
            continue;
        } else if (opcode == 16u) { /* bc: restricted CR-only conditional branch */
            const unsigned bo = (ins >> 21) & 31u;
            const unsigned bi = (ins >> 16) & 31u;
            if ((ins & 3u) != 0u || (bo != 12u && bo != 4u)) {
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            int32_t displacement = (int32_t)(ins & 0x0000FFFCu);
            if ((displacement & 0x00008000) != 0) {
                displacement |= (int32_t)0xFFFF0000u;
            }
            const int64_t target = (int64_t)pc + displacement / 4;
            if (target < 0 || target >= (int64_t)count) {
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            const unsigned cr_bit =
                (condition[bi / 4u] >> (3u - (bi % 4u))) & 1u;
            const int take = (bo == 12u) ? (cr_bit != 0u) : (cr_bit == 0u);
            if (take) {
                pc = (size_t)target;
                continue;
            }
        } else if (opcode == 11u) { /* cmpwi: signed 32-bit compare to SI */
            const unsigned bf = (ins >> 23) & 7u;
            const unsigned ra_cmp = (ins >> 16) & 31u;
            const int32_t lhs = (int32_t)(uint32_t)registers[ra_cmp];
            const int32_t rhs = (int16_t)(ins & 0xffffu);
            if (((ins >> 21) & 1u) != 0u) return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            condition[bf] = lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
        } else if (opcode == 7u) { /* MULLI: signed 16-bit immediate */
            const int64_t imm = (int16_t)(ins & 0xffffu);
            registers[rt] = registers[ra] * (uint64_t)imm;
        } else if (opcode == 14u) { /* ADDI: sign-extended 16-bit immediate */
            const int64_t imm = (int16_t)(ins & 0xffffu);
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)imm;
        } else if (opcode == 24u) { /* ORI: zero-extended immediate */
            registers[ra] = registers[rt] | (uint64_t)(ins & 0xffffu);
        } else if (opcode == 26u) { /* XORI: zero-extended immediate */
            registers[ra] = registers[rt] ^ (uint64_t)(ins & 0xffffu);
        } else if (opcode == 31u) {
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if (xo == 266u) { /* ADD */
                registers[rt] = registers[ra] + registers[rb];
            } else if (xo == 40u) { /* SUBF */
                registers[rt] = registers[rb] - registers[ra];
            } else if (xo == 233u) { /* MULLD, record form is a distinct XO */
                registers[rt] = registers[ra] * registers[rb];
            } else if (xo == 444u) { /* OR: RS field aliases RT in this decoder */
                registers[ra] = registers[rt] | registers[rb];
            } else if (xo == 28u) { /* AND */
                registers[ra] = registers[rt] & registers[rb];
            } else if (xo == 316u) { /* XOR */
                registers[ra] = registers[rt] ^ registers[rb];
            } else {
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
        } else {
            return PPC_CPU_UNSUPPORTED_INSTRUCTION;
        }
        /* PowerPC GPR0 is writable; ADDI with RA=0 specifically reads zero. */
        ++pc;
    }
    return PPC_CPU_OK;
}
