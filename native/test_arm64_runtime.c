#include "arm64_runtime.h"

#include <stdint.h>
#include <stdio.h>

int main(void) {
    const uint8_t dummy_code[] = {0, 0, 0, 0};
    uint64_t result = 0;

    if (arm64_execute_u64(NULL, sizeof(dummy_code), &result) !=
        ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("null-code check failed\n", stderr);
        return 1;
    }
    if (arm64_execute_u64(dummy_code, sizeof(dummy_code), NULL) !=
        ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("null-result check failed\n", stderr);
        return 1;
    }
    if (arm64_execute_u64(dummy_code, 0, &result) !=
        ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("zero-size check failed\n", stderr);
        return 1;
    }
    if (arm64_execute_u64(dummy_code, 3, &result) !=
        ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("unaligned-size check failed\n", stderr);
        return 1;
    }
    if (arm64_execute_u64(dummy_code, 4097, &result) !=
        ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("oversized-code check failed\n", stderr);
        return 1;
    }

#if !defined(__aarch64__)
    if (arm64_execute_u64(dummy_code, sizeof(dummy_code), &result) !=
        ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        fputs("unsupported-architecture check failed\n", stderr);
        return 1;
    }
    puts("PASS: argument validation; execution skipped on non-AArch64 host");
    return 0;
#else
    /*
     * Fixed, trusted ARM64 sequence shared with the Android bridge and
     * Python encoder regression test:
     *   mov x0, #7; mov x1, #19; sub x0, x1, x0; ret
     * Expected result: 12. Never source executable bytes from game files.
     */
    const uint8_t code[] = {
        0xE0, 0x00, 0x80, 0xD2,
        0x61, 0x02, 0x80, 0xD2,
        0x20, 0x00, 0x00, 0xCB,
        0xC0, 0x03, 0x5F, 0xD6
    };
    int status = arm64_execute_u64(code, sizeof(code), &result);
    if (status != ARM64_RUNTIME_OK || result != 12) {
        fprintf(stderr, "native execution failed: status=%d result=%llu\\n",
                status, (unsigned long long)result);
        return 1;
    }
    puts("PASS: argument validation and ARM64 code returned 12 (19 - 7)");
    return 0;
#endif
}
