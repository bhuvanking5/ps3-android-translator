#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"

/*
 * Fixed, built-in ARM64 integration test only. Never pass game-file bytes
 * to this runtime.
 */
static const uint32_t kSmokeTestCode[] = {
    0xD28000E0u, /* mov x0, #7 */
    0xD2800261u, /* mov x1, #19 */
    0xCB000020u, /* sub x0, x1, x0 */
    0xD65F03C0u  /* ret */
};

/*
 * Tiny, bounded PowerPC64 reference interpreter for two ADDI instructions.
 * It intentionally accepts only this instruction family and a fixed
 * instruction count; this is a CPU milestone test, not a PS3 game loader.
 */
static int run_ppc_addi_smoke_test(uint64_t *result) {
    static const uint32_t code[] = {
        (14u << 26) | (3u << 21) | 7u,              /* addi r3, r0, 7 */
        (14u << 26) | (4u << 21) | (3u << 16) | 5u /* addi r4, r3, 5 */
    };
    uint64_t gpr[32] = {0};
    const unsigned count = (unsigned)(sizeof(code) / sizeof(code[0]));
    if (result == 0 || count != 2u) return 0;

    for (unsigned i = 0; i < count; ++i) {
        uint32_t ins = code[i];
        unsigned opcode = ins >> 26;
        unsigned rt = (ins >> 21) & 31u;
        unsigned ra = (ins >> 16) & 31u;
        int32_t imm = (int16_t)(ins & 0xffffu);
        if (opcode != 14u) return 0;
        uint64_t base = (ra == 0u) ? 0u : gpr[ra];
        gpr[rt] = base + (uint64_t)(int64_t)imm;
    }
    *result = gpr[4];
    return *result == 12u && gpr[3] == 7u;
}

JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeStatus(JNIEnv *env, jclass clazz) {
    (void)clazz;
    uint64_t arm_result = 0;
    uint64_t ppc_result = 0;
    const int arm_status = arm64_execute_u64(
        (const uint8_t *)kSmokeTestCode, sizeof(kSmokeTestCode), &arm_result);
    const int ppc_ok = run_ppc_addi_smoke_test(&ppc_result);

    const char *message;
    if (ppc_ok && arm_status == ARM64_RUNTIME_OK && arm_result == 12u) {
        message = "Native tests PASSED: ARM64 (19 - 7 = 12) and PowerPC ADDI (7 + 5 = 12). This is a tiny CPU scaffold; PS3 games are not supported yet.";
    } else if (!ppc_ok) {
        message = "PowerPC ADDI scaffold test FAILED. PS3 games are not supported yet.";
    } else if (arm_status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        message = "PowerPC ADDI scaffold PASSED; ARM64 execution is unsupported on this device. PS3 games are not supported yet.";
    } else {
        message = "PowerPC ADDI scaffold PASSED; ARM64 smoke test failed. PS3 games are not supported yet.";
    }
    return (*env)->NewStringUTF(env, message);
}
