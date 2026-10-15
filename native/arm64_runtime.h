#ifndef PS3_ANDROID_TRANSLATOR_ARM64_RUNTIME_H
#define PS3_ANDROID_TRANSLATOR_ARM64_RUNTIME_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum {
    ARM64_RUNTIME_OK = 0,
    ARM64_RUNTIME_INVALID_ARGUMENT = 1,
    ARM64_RUNTIME_SYSTEM_ERROR = 2,
    ARM64_RUNTIME_UNSUPPORTED_ARCH = 3
};

/*
 * Execute a small AArch64 function that takes no arguments and returns a
 * uint64_t in x0. Code must be valid for the current process and ABI.
 * This is a low-level prototype, not a sandbox for untrusted code.
 */
int arm64_execute_u64(const uint8_t *code, size_t code_size, uint64_t *result);

#ifdef __cplusplus
}
#endif

#endif
