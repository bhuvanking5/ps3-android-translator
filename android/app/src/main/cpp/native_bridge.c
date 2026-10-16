#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"
#include "ppc_cpu.h"
#include "ppc_to_arm64.h"

/* Fixed, built-in ARM64 runtime smoke test; never use game-file bytes. */
static const uint32_t kArm64SmokeTest[] = {
    0xD28000E0u, /* mov x0, #7 */
    0xD2800261u, /* mov x1, #19 */
    0xCB000020u, /* sub x0, x1, x0 */
    0xD65F03C0u  /* ret */
};

/*
 * This fixed PowerPC program is the only input to this bridge's translator.
 * It is not loaded from a selected file or a PS3 game.
 */
static const uint32_t kPpcSmokeTest[] = {
    (14u << 26) | (3u << 21) | 7u, /* addi r3, r0, 7 */
    (14u << 26) | (4u << 21) | (3u << 16) | 5u, /* addi r4, r3, 5 */
    (31u << 26) | (5u << 21) | (3u << 16) |
        (4u << 11) | (266u << 1), /* add r5, r3, r4 */
    (31u << 26) | (6u << 21) | (3u << 16) |
        (5u << 11) | (40u << 1) /* subf r6, r3, r5 */
};

static int run_ppc_reference(uint64_t *result) {
    uint64_t gpr[32] = {0};
    if (result == 0) return 0;
    if (ppc_cpu_run(kPpcSmokeTest,
            sizeof(kPpcSmokeTest) / sizeof(kPpcSmokeTest[0]), gpr)
            != PPC_CPU_OK) {
        return 0;
    }
    *result = gpr[6];
    return gpr[3] == 7u && gpr[4] == 12u &&
           gpr[5] == 19u && *result == 12u;
}

JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeStatus(JNIEnv *env, jclass clazz) {
    (void)clazz;
    uint64_t arm_smoke_result = 0;
    uint64_t ppc_reference_result = 0;
    uint64_t translated_result = 0;

    const int arm_smoke_status = arm64_execute_u64(
        (const uint8_t *)kArm64SmokeTest, sizeof(kArm64SmokeTest),
        &arm_smoke_result);
    const int ppc_reference_ok = run_ppc_reference(&ppc_reference_result);

    uint32_t translated_words[PPC_TO_ARM64_MAX_WORDS] = {0};
    size_t translated_count = 0;
    const ppc_to_arm64_status translation_status = ppc_translate_to_arm64(
        kPpcSmokeTest,
        sizeof(kPpcSmokeTest) / sizeof(kPpcSmokeTest[0]),
        6u, translated_words, PPC_TO_ARM64_MAX_WORDS, &translated_count);

    int translated_status = ARM64_RUNTIME_INVALID_ARGUMENT;
    if (translation_status == PPC_TO_ARM64_OK) {
        translated_status = arm64_execute_u64(
            (const uint8_t *)translated_words,
            translated_count * sizeof(translated_words[0]),
            &translated_result);
    }

    const char *message;
    if (!ppc_reference_ok) {
        message = "PowerPC reference test FAILED. PS3 games are not supported yet.";
    } else if (translation_status != PPC_TO_ARM64_OK) {
        message = "PowerPC-to-ARM64 translation test FAILED. PS3 games are not supported yet.";
    } else if (arm_smoke_status == ARM64_RUNTIME_UNSUPPORTED_ARCH ||
               translated_status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        message = "PowerPC reference and translation PASSED; machine-code execution requires ARM64. PS3 games are not supported yet.";
    } else if (arm_smoke_status == ARM64_RUNTIME_OK &&
               translated_status == ARM64_RUNTIME_OK &&
               arm_smoke_result == 12u &&
               translated_result == ppc_reference_result) {
        message = "PASS: fixed PowerPC ADDI/ADD/SUBF translated to ARM64 and executed; result matched reference (12). Tiny test only; no PS3 games supported yet.";
    } else {
        message = "Native translation/execution comparison FAILED. PS3 games are not supported yet.";
    }
    return (*env)->NewStringUTF(env, message);
}
