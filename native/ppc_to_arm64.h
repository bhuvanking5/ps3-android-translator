#ifndef PPC_TO_ARM64_H
#define PPC_TO_ARM64_H

#include <stddef.h>
#include <stdint.h>

/* Covers the worst-case cmpwi expansion (11 words per instruction) plus setup. */
#define PPC_TO_ARM64_MAX_WORDS 3200u

typedef enum {
    PPC_TO_ARM64_OK = 0,
    PPC_TO_ARM64_INVALID_ARGUMENT,
    PPC_TO_ARM64_INSTRUCTION_LIMIT,
    PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION,
    PPC_TO_ARM64_OUTPUT_TOO_SMALL
} ppc_to_arm64_status;

/*
 * Translate a deliberately small PowerPC subset into AArch64 words.
 * Supports arithmetic/logical instructions including ori/xori immediates and signed
 * immediate `mulli`, plus signed-word cmpwi, local relative
 * unconditional b, and bc for BO=4/12 using a selected CR bit. Branch targets
 * must be inside the supplied sequence; absolute/linked branches and other BO
 * forms are rejected. Local branches are relocated after offsets are known.
 * The 512-byte frame stores 32 GPRs and all 32 modeled CR bits; x16/x17 are
 * temporary scratch registers. Initial PowerPC GPRs are zero. The generated
 * function returns the selected GPR in x0 and restores SP before returning.
 * This is research code, not a sandbox or a PS3 game translator.
 */
ppc_to_arm64_status ppc_translate_to_arm64(
    const uint32_t *instructions, size_t count,
    unsigned result_gpr, uint32_t *output, size_t output_capacity,
    size_t *output_count);

#endif
