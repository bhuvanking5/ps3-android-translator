#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"
#include "ppc_cpu.h"

/* Fixed, built-in ARM64 integration test only. Never pass game-file bytes
 * to this runtime. */
static const uint32_t kSmokeTestCode[] = {
    0xD28000E0u, /* mov x0, #7 */
    0xD2800261u, /* mov x1, #19 */
    0xCB000020u, /* sub x0, x1, x0 */
    0xD65F03C0u  /* ret */
};

/*
 * Fixed built-in PowerPC smoke test. Only the bounded interpreter subset is
 * exercised; no bytes from selected game files are passed to it.
 */
static int run_ppc_smoke_test(uint64_t *result) {
    static const uint32_t code[] = {
        (14u << 26) | (3u << 21) | 7u,
        (14u << 26) | (4u << 21) | (3u << 16) | 5u,
        (31u << 26) | (5u << 21) | (3u << 16) | (4u << 11) | (266u << 1),
        (31u << 26) | (6u << 21) | (3u << 16) | (5u << 11) | (40u << 1)
    };
    uint64_t gpr[32] = {0};
    if (result == 0) return 0;
    if (ppc_cpu_run(code, sizeof(code) / sizeof(code[0]), gpr) != PPC_CPU_OK)
        return 0;
    *result = gpr[6];
    return gpr[3] == 7u && gpr[4] == 12u
        && gpr[5] == 19u && *result == 12u;
}

JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeStatus(JNIEnv *env, jclass clazz) {
    (void)clazz;
    uint64_t arm_result = 0;
    uint64_t ppc_result = 0;
    const int arm_status = arm64_execute_u64(
        (const uint8_t *)kSmokeTestCode, sizeof(kSmokeTestCode), &arm_result);
    const int ppc_ok = run_ppc_smoke_test(&ppc_result);

    const char *message;
    if (ppc_ok && arm_status == ARM64_RUNTIME_OK && arm_result == 12u) {
        message = "Native tests PASSED: ARM64 and PowerPC ADDI/ADD/SUBF (expected result 12). Tiny CPU scaffold only; PS3 games are not supported yet.";
    } else if (!ppc_ok) {
        message = "PowerPC CPU scaffold test FAILED. PS3 games are not supported yet.";
    } else if (arm_status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        message = "PowerPC ADDI/ADD/SUBF scaffold PASSED; ARM64 execution is unsupported on this device. PS3 games are not supported yet.";
    } else {
        message = "PowerPC CPU scaffold PASSED; ARM64 smoke test failed. PS3 games are not supported yet.";
    }
    return (*env)->NewStringUTF(env, message);
}
