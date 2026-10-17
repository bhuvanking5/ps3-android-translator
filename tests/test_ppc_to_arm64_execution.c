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

    const uint32_t branch_program[] = {
        (14u << 26) | (3u << 21) | 1u,
        (18u << 26) | 8u, /* b +8: skip the next PowerPC instruction */
        (14u << 26) | (3u << 21) | 99u,
        (14u << 26) | (4u << 21) | 7u
    };
    if (check_program("local unconditional branch", branch_program,
            sizeof(branch_program) / sizeof(branch_program[0]), 3u, 1u) != 0) {
        return 1;
    }

    const uint32_t conditional_taken_program[] = {
        (14u << 26) | (3u << 21) | 7u,
        (11u << 26) | (3u << 16) | 7u, /* cmpwi cr0,r3,7 */
        (16u << 26) | (12u << 21) | (2u << 16) | 8u, /* beq +8 */
        (14u << 26) | (4u << 21) | 99u,
        (14u << 26) | (5u << 21) | 1u
    };
    if (check_program("cmpwi/conditional branch taken", conditional_taken_program,
            sizeof(conditional_taken_program) / sizeof(conditional_taken_program[0]),
            4u, 0u) != 0) {
        return 1;
    }

    const uint32_t conditional_not_taken_program[] = {
        (14u << 26) | (3u << 21) | 7u,
        (11u << 26) | (3u << 16) | 8u, /* cmpwi cr0,r3,8: EQ=false */
        (16u << 26) | (12u << 21) | (2u << 16) | 8u,
        (14u << 26) | (4u << 21) | 99u,
        (14u << 26) | (5u << 21) | 1u
    };
    if (check_program("cmpwi/conditional branch not taken", conditional_not_taken_program,
            sizeof(conditional_not_taken_program) / sizeof(conditional_not_taken_program[0]),
            4u, 99u) != 0) {
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
            7u, 0x0FF0u) != 0) {
        return 1;
    }

    const uint32_t mulli_program[] = {
        (14u << 26) | (3u << 21) | 12u,
        (7u << 26) | (4u << 21) | (3u << 16) | 5u, /* mulli r4,r3,5 */
        (7u << 26) | (5u << 21) | (4u << 16) | ((uint32_t)-3 & 0xffffu)
    };
    if (check_program("MULLI positive/negative immediate", mulli_program,
            sizeof(mulli_program) / sizeof(mulli_program[0]), 5u, UINT64_MAX - 179u) != 0) {
        return 1;
    }

    const uint32_t logical_immediate_program[] = {
        (14u << 26) | (3u << 21) | 0x0F0Fu,
        (24u << 26) | (3u << 21) | (4u << 16) | 0x00F0u, /* ori r4,r3,0x00f0 */
        (26u << 26) | (4u << 21) | (5u << 16) | 0x0F00u /* xori r5,r4,0x0f00 */
    };
    if (check_program("ORI/XORI immediate", logical_immediate_program,
            sizeof(logical_immediate_program) / sizeof(logical_immediate_program[0]),
            5u, (0x0F0Fu | 0x00F0u) ^ 0x0F00u) != 0) {
        return 1;
    }

    const uint32_t addis_program[] = {
        (14u << 26) | (3u << 21) | 0x1000u,
        (15u << 26) | (4u << 21) | (3u << 16) | 0xFFFEu,
        (15u << 26) | (5u << 21) | 0x1234u
    };
    if (check_program("ADDIS signed shifted immediates", addis_program,
            sizeof(addis_program) / sizeof(addis_program[0]), 5u,
            0x12340000u) != 0) {
        return 1;
    }

    /* Negative ADDIS must sign-extend before shifting, including RA != 0. */
    const uint32_t addis_negative_program[] = {
        (14u << 26) | (3u << 21) | 0x1000u,
        (15u << 26) | (4u << 21) | (3u << 16) | 0xFFFEu
    };
    if (check_program("ADDIS negative shifted immediate",
            addis_negative_program,
            sizeof(addis_negative_program) / sizeof(addis_negative_program[0]),
            4u, UINT64_MAX - 126975u) != 0) {
        return 1;
    }

    const uint32_t shifted_logical_program[] = {
        (14u << 26) | (3u << 21) | 0x1234u,
        (25u << 26) | (3u << 21) | (4u << 16) | 0xABCDu,
        (27u << 26) | (4u << 21) | (5u << 16) | 0x00FFu
    };
    if (check_program("ORIS/XORIS shifted immediates", shifted_logical_program,
            sizeof(shifted_logical_program) / sizeof(shifted_logical_program[0]),
            5u, (0xABCD1234u ^ 0x00FF0000u)) != 0) {
        return 1;
    }

    const uint32_t spilled_register_program[] = {
        (14u << 26) | (31u << 21) | 12000u, /* addi r31,r0,12000 */
        (14u << 26) | (30u << 21) | (31u << 16) | ((uint32_t)-5000 & 0xffffu),
        (31u << 26) | (29u << 21) | (31u << 16) |
            (30u << 11) | (40u << 1) /* subf r29,r31,r30 => -5000 */
    };
    if (check_program("r29-r31 register spill/load", spilled_register_program,
            sizeof(spilled_register_program) / sizeof(spilled_register_program[0]),
            29u, UINT64_MAX - 4999u) != 0) {
        return 1;
    }

    return 0;
}
