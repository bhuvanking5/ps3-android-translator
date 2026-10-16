#ifndef PPC_TO_ARM64_H
#define PPC_TO_ARM64_H

#include <stddef.h>
#include <stdint.h>

#define PPC_TO_ARM64_MAX_WORDS 258u

typedef enum {
    PPC_TO_ARM64_OK = 0,
    PPC_TO_ARM64_INVALID_ARGUMENT,
    PPC_TO_ARM64_INSTRUCTION_LIMIT,
    PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION,
    PPC_TO_ARM64_OUTPUT_TOO_SMALL
} ppc_to_arm64_status;

/*
 * Translate a deliberately tiny PowerPC subset into AArch64 words.
 * GPRs r0-r17 map to x0-x17 only (x18/platform and callee-saved registers
 * are excluded). The generated function returns the selected PPC GPR in x0.
 * This is research code, not a sandbox or a PS3 game translator.
 */
ppc_to_arm64_status ppc_translate_to_arm64(
    const uint32_t *instructions, size_t count,
    unsigned result_gpr, uint32_t *output, size_t output_capacity,
    size_t *output_count);

#endif
