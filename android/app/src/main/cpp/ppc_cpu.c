#include "ppc_cpu.h"

ppc_cpu_status ppc_cpu_run_with_memory(
    const uint32_t *instructions, size_t count, uint64_t registers[32],
    ppc_cpu_read32_fn read32, ppc_cpu_write32_fn write32,
    void *memory_context) {
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
            /* AA=1 is absolute addressing; LK=1 links. Neither is implemented. */
            if ((ins & 3u) != 0u)
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            int32_t displacement = (int32_t)(ins & 0x03FFFFFCu);
            if ((displacement & 0x02000000) != 0)
                displacement |= (int32_t)0xFC000000u;
            const int64_t target = (int64_t)pc + displacement / 4;
            if (target < 0 || target >= (int64_t)count)
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            pc = (size_t)target;
            continue;
        } else if (opcode == 16u) { /* restricted CR-only conditional branch */
            const unsigned bo = (ins >> 21) & 31u;
            const unsigned bi = (ins >> 16) & 31u;
            /* AA=1 and LK=1 are not supported by this bounded runner. */
            if ((ins & 3u) != 0u ||
                (bo != 12u && bo != 4u))
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            const unsigned cr_bit =
                (condition[bi / 4u] >> (3u - (bi % 4u))) & 1u;
            const int take = (bo == 12u) ? (cr_bit != 0u) : (cr_bit == 0u);
            /* An untaken branch falls through; its target is never used. */
            if (take) {
                int32_t displacement = (int32_t)(ins & 0x0000FFFCu);
                if ((displacement & 0x00008000) != 0)
                    displacement |= (int32_t)0xFFFF0000u;
                const int64_t target = (int64_t)pc + displacement / 4;
                if (target < 0 || target >= (int64_t)count)
                    return PPC_CPU_UNSUPPORTED_INSTRUCTION;
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
        } else if (opcode == 32u) { /* LWZ: callback-backed 32-bit guest read */
            if (read32 == 0) return PPC_CPU_MEMORY_ACCESS_ERROR;
            const int32_t displacement = (int16_t)(ins & 0xffffu);
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            const uint32_t address = (uint32_t)(base + (uint64_t)displacement);
            uint32_t value = 0u;
            if (!read32(memory_context, address, &value))
                return PPC_CPU_MEMORY_ACCESS_ERROR;
            registers[rt] = (uint64_t)value;
        } else if (opcode == 36u) { /* STW: callback-backed 32-bit guest write */
            if (write32 == 0) return PPC_CPU_MEMORY_ACCESS_ERROR;
            const int32_t displacement = (int16_t)(ins & 0xffffu);
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            const uint32_t address = (uint32_t)(base + (uint64_t)displacement);
            if (!write32(memory_context, address, (uint32_t)registers[rt]))
                return PPC_CPU_MEMORY_ACCESS_ERROR;
        } else if (opcode == 15u) { /* ADDIS: signed immediate shifted left 16 */
            const int64_t imm = (int64_t)(int16_t)(ins & 0xffffu) * 65536;
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)imm;
        } else if (opcode == 7u) { /* MULLI: signed 16-bit immediate */
            const int64_t imm = (int16_t)(ins & 0xffffu);
            registers[rt] = registers[ra] * (uint64_t)imm;
        } else if (opcode == 14u) { /* ADDI: sign-extended 16-bit immediate */
            const int64_t imm = (int16_t)(ins & 0xffffu);
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)imm;
        } else if (opcode == 24u || opcode == 25u) { /* ORI / ORIS */
            const uint64_t immediate = (uint64_t)(ins & 0xffffu) <<
                                       (opcode == 25u ? 16u : 0u);
            registers[ra] = registers[rt] | immediate;
        } else if (opcode == 26u || opcode == 27u) { /* XORI / XORIS */
            const uint64_t immediate = (uint64_t)(ins & 0xffffu) <<
                                       (opcode == 27u ? 16u : 0u);
            registers[ra] = registers[rt] ^ immediate;
        } else if (opcode == 21u) { /* RLWINM: rotate left word then apply wrapped bit mask */
            const unsigned shift = (ins >> 11) & 31u;
            const unsigned mb = (ins >> 6) & 31u;
            const unsigned me = (ins >> 1) & 31u;
            const uint32_t source = (uint32_t)registers[rt];
            const uint32_t rotated = shift == 0u ? source :
                (uint32_t)((source << shift) | (source >> (32u - shift)));
            uint32_t mask = 0u;
            unsigned bit = mb;
            for (;;) {
                mask |= (uint32_t)1u << (31u - bit);
                if (bit == me) break;
                bit = (bit + 1u) & 31u;
            }
            registers[ra] = (uint64_t)(rotated & mask);
        } else if (opcode == 28u) { /* ANDI.: logical immediate, updates CR0 */
            const uint64_t value = registers[rt] & (uint64_t)(ins & 0xffffu);
            registers[ra] = value;
            /* The 16-bit immediate makes the result non-negative: EQ if zero, GT otherwise. */
            condition[0] = value == 0u ? 2u : 4u;
        } else if (opcode == 31u) {
            /* Rc=1 requires CR0 updates, which this subset does not implement. */
            if ((ins & 1u) != 0u) return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if (xo == 266u) registers[rt] = registers[ra] + registers[rb];
            else if (xo == 40u) registers[rt] = registers[rb] - registers[ra];
            else if (xo == 233u) registers[rt] = registers[ra] * registers[rb];
            else if (xo == 444u) registers[ra] = registers[rt] | registers[rb];
            else if (xo == 28u) registers[ra] = registers[rt] & registers[rb];
            else if (xo == 316u) registers[ra] = registers[rt] ^ registers[rb];
            else return PPC_CPU_UNSUPPORTED_INSTRUCTION;
        } else {
            return PPC_CPU_UNSUPPORTED_INSTRUCTION;
        }
        ++pc;
    }
    return PPC_CPU_OK;
}

ppc_cpu_status ppc_cpu_run_with_read32(
    const uint32_t *instructions, size_t count, uint64_t registers[32],
    ppc_cpu_read32_fn read32, void *read_context) {
    return ppc_cpu_run_with_memory(instructions, count, registers,
        read32, 0, read_context);
}

ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]) {
    return ppc_cpu_run_with_memory(instructions, count, registers, 0, 0, 0);
}
