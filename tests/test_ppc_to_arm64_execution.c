#include "arm64_runtime.h"
#include "ppc_cpu.h"
#include "ppc_to_arm64.h"

#include <stdio.h>
#include <stdint.h>

int main(void) {
    const uint32_t ppc_program[] = {
        (14u << 26) | (3u << 21) | 7u,
        (14u << 26) | (4u << 21) | (3u << 16) | 5u,
        (31u << 26) | (5u << 21) | (3u << 16) |
            (4u << 11) | (266u << 1),
        (31u << 26) | (6u << 21) | (3u << 16) |
            (5u << 11) | (40u << 1)
    };
    uint64_t gpr[32] = {0};
    if (ppc_cpu_run(ppc_program,
            sizeof(ppc_program) / sizeof(ppc_program[0]), gpr) != PPC_CPU_OK ||
        gpr[6] != 12u) {
        fputs("FAIL: PowerPC reference result was not 12\n", stderr);
        return 1;
    }

    uint32_t arm64_words[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t arm64_count = 0;
    if (ppc_translate_to_arm64(ppc_program,
            sizeof(ppc_program) / sizeof(ppc_program[0]), 6u,
            arm64_words, PPC_TO_ARM64_MAX_WORDS, &arm64_count)
            != PPC_TO_ARM64_OK) {
        fputs("FAIL: PowerPC program did not translate\n", stderr);
        return 1;
    }

    uint64_t translated_result = 0;
    const int status = arm64_execute_u64(
        (const uint8_t *)arm64_words,
        arm64_count * sizeof(arm64_words[0]), &translated_result);

#if defined(__aarch64__)
    if (status != ARM64_RUNTIME_OK || translated_result != gpr[6]) {
        fprintf(stderr, "FAIL: status=%d translated=%llu reference=%llu\n",
                status, (unsigned long long)translated_result,
                (unsigned long long)gpr[6]);
        return 1;
    }
    puts("PASS: generated ARM64 executed; result matches PowerPC reference (12)");
#else
    if (status != ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        fprintf(stderr, "FAIL: expected unsupported-architecture status, got %d\n",
                status);
        return 1;
    }
    puts("PASS: translation matches expected words; execution requires AArch64");
#endif
    return 0;
}
