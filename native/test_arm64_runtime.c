#include "arm64_runtime.h"

#include <stdint.h>
#include <stdio.h>

int main(void) {
#if !defined(__aarch64__)
    puts("SKIP: native execution test requires an AArch64 runner");
    return 0;
#else
    /* mov x0, #42; ret — AArch64 little-endian instruction bytes. */
    const uint8_t code[] = {
        0x40, 0x05, 0x80, 0xD2,
        0xC0, 0x03, 0x5F, 0xD6
    };
    uint64_t result = 0;
    int status = arm64_execute_u64(code, sizeof(code), &result);
    if (status != ARM64_RUNTIME_OK || result != 42) {
        fprintf(stderr, "native execution failed: status=%d result=%llu\n",
                status, (unsigned long long)result);
        return 1;
    }

    if (arm64_execute_u64(code, 0, &result) != ARM64_RUNTIME_INVALID_ARGUMENT) {
        fputs("invalid-size check failed\n", stderr);
        return 1;
    }

    puts("PASS: executed ARM64 machine code and returned 42");
    return 0;
#endif
}
