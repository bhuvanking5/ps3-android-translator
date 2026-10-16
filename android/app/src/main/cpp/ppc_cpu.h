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
    PPC_CPU_UNSUPPORTED_INSTRUCTION,
    PPC_CPU_MEMORY_ACCESS_ERROR
} ppc_cpu_status;

/* Return nonzero on a successful 32-bit guest read, zero on an unmapped read. */
typedef int (*ppc_cpu_read32_fn)(void *context, uint32_t address,
                                 uint32_t *value);

/* Runs the bounded instruction subset without guest memory access. */
ppc_cpu_status ppc_cpu_run(const uint32_t *instructions, size_t count,
                           uint64_t registers[32]);

/*
 * Same bounded subset with an optional guest read callback for lwz.
 * The callback is the only memory path; there is no general guest RAM here.
 */
ppc_cpu_status ppc_cpu_run_with_read32(
    const uint32_t *instructions, size_t count, uint64_t registers[32],
    ppc_cpu_read32_fn read32, void *read_context);

#endif
