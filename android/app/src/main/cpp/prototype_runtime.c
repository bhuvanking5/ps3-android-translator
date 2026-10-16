#include "prototype_runtime.h"
#include "prototype_guest_io.h"

static int prototype_read32(void *context, uint32_t address,
                            uint32_t *value) {
    const ps3_controller_report *report =
        (const ps3_controller_report *)context;
    return ps3_prototype_guest_read32(report, address, value) ==
        PS3_GUEST_IO_OK;
}

/*
 * Runs a bounded CPU slice against one stable controller snapshot. Guest
 * reads are limited to the synthetic controller map; this is not guest RAM
 * or a model of real PS3 MMIO.
 */
ppc_cpu_status ps3_prototype_run_slice(const uint32_t *instructions,
    size_t instruction_count, uint64_t registers[32],
    ps3_controller_report *controller_out) {
    if (controller_out == 0) return PPC_CPU_INVALID_ARGUMENT;
    *controller_out = ps3_controller_input_read_report();
    return ppc_cpu_run_with_read32(instructions, instruction_count,
        registers, prototype_read32, controller_out);
}
