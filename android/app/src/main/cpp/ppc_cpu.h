#ifndef PPC_CPU_H
#define PPC_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PPC_CPU_MAX_INSTRUCTIONS 256u

typedef enum {
    PPC_CPU_OK = 0,
    PPC_CPU_INVALID_ARGUMENT,
    PPC_CPU_INSTRUCTION_LIMIT,
    PPC_CPU_UNSUPPORTED_INSTRUCTION
} ppc_cpu_status;

/* Runs a bounded, in-memory PowerPC subset. This is not a PS3 game runner. */
ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]);

#endif
