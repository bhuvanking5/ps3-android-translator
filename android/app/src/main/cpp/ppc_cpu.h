#ifndef PPC_CPU_H
#define PPC_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PPC_CPU_MAX_INSTRUCTIONS 256u
#define PPC_CPU_MAX_STEPS 4096u

typedef enum {
    PPC_CPU_OK = 0,
    PPC_CPU_INVALID_ARGUMENT,
    PPC_CPU_INSTRUCTION_LIMIT,
    PPC_CPU_STEP_LIMIT,
    PPC_CPU_UNSUPPORTED_INSTRUCTION
} ppc_cpu_status;

/* Runs a bounded in-memory PowerPC subset: arithmetic/logical operations including signed-immediate `mulli`,
 * local relative b, signed-word cmpwi, and local bc for BO=4 or BO=12.
 * Conditional branches use the selected CR bit; unsupported BO forms,
 * absolute/linked branches, and out-of-sequence targets are rejected.
 * Execution is capped by PPC_CPU_MAX_STEPS. This is not a PS3 game runner. */
ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]);

#endif
