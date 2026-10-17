#include "ppc_cpu.h"
#include "guest_memory.h"

static int32_t sign16(uint32_t value) { return (int16_t)(value & 0xffffu); }

ppc_cpu_status ppc_cpu_run_guest(void *memory_context, uint32_t entry_point,
    uint64_t registers[32], size_t max_steps, size_t *steps_executed) {
    if (memory_context == 0 || registers == 0 || max_steps == 0u)
        return PPC_CPU_INVALID_ARGUMENT;
    uint32_t pc = entry_point;
    uint64_t lr = UINT64_MAX; /* Synthetic top-level return sentinel. */
    uint64_t ctr = 0u;
    uint8_t condition[8] = {0};
    size_t steps = 0u;
    while (steps < max_steps) {
        uint32_t ins = 0u;
        if (!ps3_guest_memory_fetch32(memory_context, pc, &ins)) {
            if (steps_executed != 0) *steps_executed = steps;
            return PPC_CPU_MEMORY_ACCESS_ERROR;
        }
        ++steps;
        const unsigned opcode = ins >> 26;
        const unsigned rt = (ins >> 21) & 31u;
        const unsigned ra = (ins >> 16) & 31u;
        const unsigned rb = (ins >> 11) & 31u;
        uint32_t next_pc = pc + 4u;

        if (opcode == 18u) {
            const int absolute = (ins & 2u) != 0u;
            const int link = (ins & 1u) != 0u;
            int32_t displacement = (int32_t)(ins & 0x03fffffcu);
            if ((displacement & 0x02000000) != 0)
                displacement |= (int32_t)0xfc000000u;
            if (link) lr = (uint64_t)(pc + 4u);
            next_pc = absolute ? (uint32_t)displacement :
                (uint32_t)(pc + (uint32_t)displacement);
        } else if (opcode == 19u) {
            const unsigned xo = (ins >> 1) & 0x3ffu;
            const unsigned bo = (ins >> 21) & 31u;
            const unsigned bi = (ins >> 16) & 31u;
            if ((xo != 16u && xo != 528u) ||
                (ins & 0x0000e000u) != 0u ||
                (bo != 20u && bo != 12u && bo != 4u)) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            int take = 1;
            if (bo != 20u) {
                const unsigned cr_bit =
                    (condition[bi / 4u] >> (3u - (bi % 4u))) & 1u;
                take = bo == 12u ? cr_bit != 0u : cr_bit == 0u;
            }
            if (take) {
                if (xo == 16u && lr == UINT64_MAX) {
                    if (steps_executed != 0) *steps_executed = steps;
                    return PPC_CPU_GUEST_EXIT;
                }
                next_pc = (uint32_t)((xo == 16u ? lr : ctr) & ~UINT64_C(3));
                if ((ins & 1u) != 0u) lr = (uint64_t)(pc + 4u);
            }
        } else if (opcode == 16u) {
            const unsigned bo = (ins >> 21) & 31u;
            const unsigned bi = (ins >> 16) & 31u;
            if ((ins & 3u) != 0u || (bo != 12u && bo != 4u)) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            int32_t displacement = (int32_t)(ins & 0xfffcu);
            if ((displacement & 0x8000) != 0) displacement |= (int32_t)0xffff0000u;
            const unsigned cr_bit = (condition[bi / 4u] >> (3u - (bi % 4u))) & 1u;
            const int take = bo == 12u ? cr_bit != 0u : cr_bit == 0u;
            if (take) next_pc = (uint32_t)(pc + (uint32_t)displacement);
        } else if (opcode == 10u || opcode == 11u) {
            /* CMPLWI compares unsigned 32-bit words; CMPWI compares signed words. */
            const unsigned bf = (ins >> 23) & 7u;
            if (((ins >> 21) & 1u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            if (opcode == 10u) {
                const uint32_t lhs = (uint32_t)registers[ra];
                const uint32_t rhs = ins & 0xffffu;
                condition[bf] = lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
            } else {
                const int32_t lhs = (int32_t)(uint32_t)registers[ra];
                const int32_t rhs = sign16(ins);
                condition[bf] = lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
            }
        } else if (opcode == 14u) {
            const uint64_t base = ra == 0u ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)(int64_t)sign16(ins);
        } else if (opcode == 15u) {
            const uint64_t base = ra == 0u ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)((int64_t)sign16(ins) * 65536);
        } else if (opcode == 24u || opcode == 25u) {
            const uint64_t immediate = (uint64_t)(ins & 0xffffu) <<
                                       (opcode == 25u ? 16u : 0u);
            registers[ra] = registers[rt] | immediate;
        } else if (opcode == 28u) {
            /* ANDI. always updates CR0; a 16-bit mask gives EQ or GT, never LT. */
            const uint64_t value = registers[rt] & (uint64_t)(ins & 0xffffu);
            registers[ra] = value;
            condition[0] = value == 0u ? 2u : 4u;
        } else if (opcode == 26u || opcode == 27u) {
            const uint64_t immediate = (uint64_t)(ins & 0xffffu) <<
                                       (opcode == 27u ? 16u : 0u);
            registers[ra] = registers[rt] ^ immediate;
        } else if (opcode == 58u || opcode == 62u) {
            /* PPC64 LD/LDU and STD/STDU use a signed DS displacement. */
            const unsigned xo = ins & 3u;
            if (xo > 1u || (xo == 1u && (ra == 0u ||
                (opcode == 58u && rt == ra)))) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            const int32_t displacement = (int16_t)(ins & 0xfffcu);
            const uint64_t base = ra == 0u ? 0u : registers[ra];
            const uint32_t address = (uint32_t)(base + (uint64_t)(int64_t)displacement);
            if (opcode == 58u) {
                uint64_t value = 0u;
                if (!ps3_guest_memory_read64(memory_context, address, &value)) {
                    if (steps_executed != 0) *steps_executed = steps;
                    return PPC_CPU_MEMORY_ACCESS_ERROR;
                }
                registers[rt] = value;
            } else if (!ps3_guest_memory_write64(memory_context, address, registers[rt])) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_MEMORY_ACCESS_ERROR;
            }
            if (xo == 1u) registers[ra] = address;
        } else if (opcode == 34u || opcode == 38u ||
                   opcode == 40u || opcode == 42u || opcode == 44u) {
            /* LBZ/STB, LHZ/LHA/STH; halfword values use big-endian memory. */
            const uint32_t address = (uint32_t)((ra == 0u ? 0u : registers[ra]) +
                (uint64_t)(int64_t)sign16(ins));
            int ok = 0;
            if (opcode == 34u) {
                uint8_t value = 0u;
                ok = ps3_guest_memory_read8(memory_context, address, &value);
                if (ok) registers[rt] = value;
            } else if (opcode == 40u || opcode == 42u) {
                uint16_t value = 0u;
                ok = ps3_guest_memory_read16(memory_context, address, &value);
                if (ok) registers[rt] = opcode == 42u
                    ? (uint64_t)(int64_t)(int16_t)value : (uint64_t)value;
            } else if (opcode == 38u) {
                ok = ps3_guest_memory_write8(memory_context, address,
                    (uint8_t)registers[rt]);
            } else {
                ok = ps3_guest_memory_write16(memory_context, address,
                    (uint16_t)registers[rt]);
            }
            if (!ok) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_MEMORY_ACCESS_ERROR;
            }
        } else if (opcode == 32u) {
            const uint32_t address = (uint32_t)((ra == 0u ? 0u : registers[ra]) +
                (uint64_t)(int64_t)sign16(ins));
            uint32_t value = 0u;
            if (!ps3_guest_memory_read32(memory_context, address, &value)) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_MEMORY_ACCESS_ERROR;
            }
            registers[rt] = value;
        } else if (opcode == 36u) {
            const uint32_t address = (uint32_t)((ra == 0u ? 0u : registers[ra]) +
                (uint64_t)(int64_t)sign16(ins));
            if (!ps3_guest_memory_write32(memory_context, address, (uint32_t)registers[rt])) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_MEMORY_ACCESS_ERROR;
            }
        } else if (opcode == 7u) {
            registers[rt] = registers[ra] * (uint64_t)(int64_t)sign16(ins);
        } else if (opcode == 21u || opcode == 23u) {
            /* RLWINM/RLWNM: rotate low 32-bit word then apply inclusive PPC mask. */
            if ((ins & 1u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            const unsigned shift = opcode == 21u
                ? ((ins >> 11) & 31u) : (unsigned)(registers[rb] & 31u);
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
        } else if (opcode == 31u) {
            /* Rc=1 requires CR0 updates, which this guest runner does not implement. */
            if ((ins & 1u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            const unsigned xo = (ins >> 1) & 0x3ffu;
            const unsigned spr = (((ins >> 11) & 31u) << 5) | ((ins >> 16) & 31u);
            if (xo == 339u && spr == 8u) registers[rt] = lr;       /* mflr */
            else if (xo == 467u && spr == 8u) lr = registers[rt]; /* mtlr */
            else if (xo == 339u && spr == 9u) registers[rt] = ctr; /* mfctr */
            else if (xo == 467u && spr == 9u) ctr = registers[rt]; /* mtctr */
            else if (xo == 266u) registers[rt] = registers[ra] + registers[rb];
            else if (xo == 40u) registers[rt] = registers[rb] - registers[ra];
            else if (xo == 233u) registers[rt] = registers[ra] * registers[rb];
            else if (xo == 444u) registers[ra] = registers[rt] | registers[rb];
            else if (xo == 28u) registers[ra] = registers[rt] & registers[rb];
            else if (xo == 24u || xo == 536u) {
                /* SLW/SRW operate on 32-bit words; counts >= 32 yield zero. */
                const uint32_t source = (uint32_t)registers[rt];
                const uint64_t shift_count = registers[rb] & 63u;
                uint32_t result = 0u;
                if (shift_count < 32u) {
                    result = xo == 24u
                        ? (uint32_t)(source << (unsigned)shift_count)
                        : (uint32_t)(source >> (unsigned)shift_count);
                }
                registers[ra] = (uint64_t)result;
            }
            else if (xo == 316u) registers[ra] = registers[rt] ^ registers[rb];
            else {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
        } else {
            if (steps_executed != 0) *steps_executed = steps;
            return PPC_CPU_UNSUPPORTED_INSTRUCTION;
        }
        pc = next_pc;
    }
    if (steps_executed != 0) *steps_executed = steps;
    return PPC_CPU_STEP_LIMIT;
}
