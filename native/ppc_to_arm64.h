#ifndef PPC_TO_ARM64_H
#define PPC_TO_ARM64_H

#include <stddef.h>
#include <stdint.h>

/* Worst case: 4 emitted words per PPC instruction plus prologue/init/return. */
#define PPC_TO_ARM64_MAX_WORDS 1100u

typedef enum {
    PPC_TO_ARM64_OK = 0,
    PPC_TO_ARM64_INVALID_ARGUMENT,
    PPC_TO_ARM64_INSTRUCTION_LIMIT,
    PPC_TO_ARM64_UNSUPPORTED_INSTRUCTION,
    PPC_TO_ARM64_OUTPUT_TOO_SMALL
} ppc_to_arm64_status;

/*
 * Translate a deliberately small PowerPC subset into AArch64 words.
 * Supports arithmetic/logical instructions plus local relative unconditional
 * branches (b) whose targets are inside the supplied sequence. Conditional,
 * absolute, linked, and out-of-sequence branches are rejected. Local branches
 * are relocated after all generated instruction offsets are known.
 * Every PowerPC GPR r0-r31 is stored in a 256-byte stack frame; x16/x17 are
 * temporary scratch registers. Initial PowerPC GPRs are zero. The generated
 * function returns the selected GPR in x0 and restores SP before returning.
 * This is research code, not a sandbox or a PS3 game translator.
 */
ppc_to_arm64_status ppc_translate_to_arm64(
    const uint32_t *instructions, size_t count,
    unsigned result_gpr, uint32_t *output, size_t output_capacity,
    size_t *output_count);

#endif
