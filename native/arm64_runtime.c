#include "arm64_runtime.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define ARM64_RUNTIME_MAX_CODE_SIZE 4096u

int arm64_execute_u64(const uint8_t *code, size_t code_size, uint64_t *result) {
    /* Validate arguments on every host, including non-AArch64 CI runners. */
    if (code == NULL || result == NULL || code_size == 0 ||
        code_size > ARM64_RUNTIME_MAX_CODE_SIZE || (code_size % 4) != 0) {
        return ARM64_RUNTIME_INVALID_ARGUMENT;
    }

#if !defined(__aarch64__)
    return ARM64_RUNTIME_UNSUPPORTED_ARCH;
#else
    long page_size = sysconf(_SC_PAGESIZE);
    if (page_size <= 0 || (size_t)page_size < code_size) {
        return ARM64_RUNTIME_SYSTEM_ERROR;
    }

    void *memory = mmap(NULL, (size_t)page_size, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (memory == MAP_FAILED) {
        return ARM64_RUNTIME_SYSTEM_ERROR;
    }

    memcpy(memory, code, code_size);
    __builtin___clear_cache((char *)memory, (char *)memory + code_size);

    /* Write first, then switch to read+execute (W^X). */
    if (mprotect(memory, (size_t)page_size, PROT_READ | PROT_EXEC) != 0) {
        munmap(memory, (size_t)page_size);
        return ARM64_RUNTIME_SYSTEM_ERROR;
    }

    typedef uint64_t (*arm64_function_t)(void);
    arm64_function_t function = (arm64_function_t)memory;
    *result = function();

    if (munmap(memory, (size_t)page_size) != 0) {
        return ARM64_RUNTIME_SYSTEM_ERROR;
    }
    return ARM64_RUNTIME_OK;
#endif
}
