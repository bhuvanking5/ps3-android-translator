#include "ppc_cpu.h"

ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]) {
    if (instructions == 0 || registers == 0) return PPC_CPU_INVALID_ARGUMENT;
    if (count == 0u || count > PPC_CPU_MAX_INSTRUCTIONS)
        return PPC_CPU_INSTRUCTION_LIMIT;

    for (size_t i = 0; i < count; ++i) {
        const uint32_t ins = instructions[i];
        const unsigned opcode = ins >> 26;
        const unsigned rt = (ins >> 21) & 31u;
        const unsigned ra = (ins >> 16) & 31u;
        const unsigned rb = (ins >> 11) & 31u;

        if (opcode == 14u) { /* ADDI: sign-extended 16-bit immediate */
            const int64_t imm = (int16_t)(ins & 0xffffu);
            const uint64_t base = (ra == 0u) ? 0u : registers[ra];
            registers[rt] = base + (uint64_t)imm;
        } else if (opcode == 31u) {
            const unsigned xo = (ins >> 1) & 0x3ffu;
            if (xo == 266u) { /* ADD */
                registers[rt] = registers[ra] + registers[rb];
            } else if (xo == 40u) { /* SUBF */
                registers[rt] = registers[rb] - registers[ra];
            } else {
                return PPC_CPU_UNSUPPORTED_INSTRUCTION;
            }
        } else {
            return PPC_CPU_UNSUPPORTED_INSTRUCTION;
        }
        /* PowerPC GPR0 is writable; ADDI with RA=0 specifically reads zero. */
    }
    return PPC_CPU_OK;
}
