#ifndef PS3_PROTOTYPE_RUNTIME_H
#define PS3_PROTOTYPE_RUNTIME_H
#include <stddef.h>
#include <stdint.h>
#include "controller_input.h"
#include "ppc_cpu.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Bounded reference-CPU slice plus a controller snapshot; not a PS3 emulator. */
ppc_cpu_status ps3_prototype_run_slice(const uint32_t *instructions, size_t instruction_count, uint64_t registers[32], ps3_controller_report *controller_out);
#ifdef __cplusplus
}
#endif
#endif
