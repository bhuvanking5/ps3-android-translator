#include "prototype_runtime.h"

/*
 * Small integration seam for exercising the bounded CPU and controller
 * mailbox together. It does not emulate PS3 system calls or game input.
 */
ppc_cpu_status ps3_prototype_run_slice(const uint32_t *instructions,
    size_t instruction_count, uint64_t registers[32],
    ps3_controller_report *controller_out) {
    if (controller_out == 0) return PPC_CPU_INVALID_ARGUMENT;
    const ppc_cpu_status status = ppc_cpu_run(
        instructions, instruction_count, registers);
    if (status != PPC_CPU_OK) return status;
    *controller_out = ps3_controller_input_read_report();
    return PPC_CPU_OK;
}
