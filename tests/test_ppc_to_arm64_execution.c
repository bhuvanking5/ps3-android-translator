#include "arm64_runtime.h"
#include "ppc_cpu.h"
#include "ppc_to_arm64.h"

#include <stdio.h>
#include <stdint.h>

static int check_program(const char *label, const uint32_t *program,
                         size_t program_count, unsigned result_gpr,
                         uint64_t expected_result) {
    uint64_t gpr[32] = {0};
    if (ppc_cpu_run(program, program_count, gpr) != PPC_CPU_OK ||
        gpr[result_gpr] != expected_result) {
        fprintf(stderr, "FAIL: %s reference result mismatch (got %llu, expected %llu)\n",
                label, (unsigned long long)gpr[result_gpr],
                (unsigned long long)expected_result);
        return 1;
    }

    uint32_t arm64_words[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t arm64_count = 0;
    if (ppc_translate_to_arm64(program, program_count, result_gpr,
            arm64_words, PPC_TO_ARM64_MAX_WORDS, &arm64_count)
            != PPC_TO_ARM64_OK) {
        fprintf(stderr, "FAIL: %s did not translate\n", label);
        return 1;
    }

    uint64_t translated_result = 0;
    const int status = arm64_execute_u64(
        (const uint8_t *)arm64_words,
        arm64_count * sizeof(arm64_words[0]), &translated_result);

#if defined(__aarch64__)
    if (status != ARM64_RUNTIME_OK || translated_result != gpr[result_gpr]) {
        fprintf(stderr, "FAIL: %s status=%d translated=%llu reference=%llu\n",
                label, status, (unsigned long long)translated_result,
                (unsigned long long)gpr[result_gpr]);
        return 1;
    }
    printf("PASS: %s generated ARM64 matches PowerPC reference (%llu)\n",
           label, (unsigned long long)translated_result);
#else
    if (status != ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        fprintf(stderr, "FAIL: %s expected unsupported-architecture status, got %d\n",
                label, status);
        return 1;
    }
    printf("PASS: %s translation/reference checks passed; execution requires AArch64\n",
           label);
#endif
    return 0;
}

int main(void) {
    const uint32_t arithmetic_program[] = {
        (14u << 26) | (3u << 21) | 7u,
        (14u << 26) | (4u << 21) | (3u << 16) | 5u,
        (31u << 26) | (5u << 21) | (3u << 16) |
            (4u << 11) | (266u << 1),
        (31u << 26) | (6u << 21) | (3u << 16) |
            (5u << 11) | (40u << 1)
    };
    if (check_program("ADDI/ADD/SUBF", arithmetic_program,
            sizeof(arithmetic_program) / sizeof(arithmetic_program[0]),
            6u, 12u) != 0) {
        return 1;
    }

    /* Build two values, then test OR, AND, and XOR destination semantics. */
    const uint32_t logical_program[] = {
        (14u << 26) | (3u << 21) | 0x0F0Fu,
        (14u << 26) | (4u << 21) | 0x00FFu,
        (31u << 26) | (3u << 21) | (5u << 16) |
            (4u << 11) | (444u << 1), /* or r5,r3,r4 */
        (31u << 26) | (3u << 21) | (6u << 16) |
            (4u << 11) | (28u << 1), /* and r6,r3,r4 */
        (31u << 26) | (3u << 21) | (7u << 16) |
            (4u << 11) | (316u << 1) /* xor r7,r3,r4 */
    };
    if (check_program("OR/AND/XOR", logical_program,
            sizeof(logical_program) / sizeof(logical_program[0]),
            7u, 0x0F00u) != 0) {
        return 1;
    }

    return 0;
}
