#include "ppc_cpu.h"
#include "guest_memory.h"

static int32_t sign16(uint32_t value) { return (int16_t)(value & 0xffffu); }

ppc_cpu_status ppc_cpu_run_guest(void *memory_context, uint32_t entry_point,
    uint64_t registers[32], size_t max_steps, size_t *steps_executed) {
    if (memory_context == 0 || registers == 0 || max_steps == 0u)
        return PPC_CPU_INVALID_ARGUMENT;
    uint32_t pc = entry_point;
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
            if ((ins & 3u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            int32_t displacement = (int32_t)(ins & 0x03fffffcu);
            if ((displacement & 0x02000000) != 0)
                displacement |= (int32_t)0xfc000000u;
            next_pc = (uint32_t)(pc + (uint32_t)displacement);
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
        } else if (opcode == 11u) {
            const unsigned bf = (ins >> 23) & 7u;
            const int32_t lhs = (int32_t)(uint32_t)registers[ra];
            const int32_t rhs = sign16(ins);
            if (((ins >> 21) & 1u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            condition[bf] = lhs < rhs ? 8u : lhs > rhs ? 4u : 2u;
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
        } else if (opcode == 26u || opcode == 27u) {
            const uint64_t immediate = (uint64_t)(ins & 0xffffu) <<
                                       (opcode == 27u ? 16u : 0u);
            registers[ra] = registers[rt] ^ immediate;
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
        } else if (opcode == 31u) {
            /* Rc=1 requires CR0 updates, which this guest runner does not implement. */
            if ((ins & 1u) != 0u) {
                if (steps_executed != 0) *steps_executed = steps;
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if (xo == 266u) registers[rt] = registers[ra] + registers[rb];
            else if (xo == 40u) registers[rt] = registers[rb] - registers[ra];
            else if (xo == 233u) registers[rt] = registers[ra] * registers[rb];
            else if (xo == 444u) registers[ra] = registers[rt] | registers[rb];
            else if (xo == 28u) registers[ra] = registers[rt] & registers[rb];
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
