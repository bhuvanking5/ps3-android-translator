#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"

/*
 * Fixed, built-in ARM64 integration test only:
 *   mov x0, #7
 *   mov x1, #19
 *   sub x0, x1, x0
 *   ret
 * Expected result: 12.
 * Never pass bytes from game files or downloads to this runtime.
 */
static const uint32_t kSmokeTestCode[] = {
    0xD28000E0u,
    0xD2800261u,
    0xCB000020u,
    0xD65F03C0u
};

JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeStatus(JNIEnv *env, jclass clazz) {
    (void)clazz;
    uint64_t result = 0;
    const int status = arm64_execute_u64(
        (const uint8_t *)kSmokeTestCode, sizeof(kSmokeTestCode), &result);

    const char *message;
    if (status == ARM64_RUNTIME_OK && result == 12) {
        message = "Native ARM64 multi-instruction test PASSED (19 - 7 = 12). PS3 games are not supported yet.";
    } else if (status == ARM64_RUNTIME_UNSUPPORTED_ARCH) {
        message = "Native runtime loaded; ARM64 execution is unsupported on this device.";
    } else {
        message = "Native runtime loaded; ARM64 multi-instruction test could not execute. PS3 games are not supported yet.";
    }
    return (*env)->NewStringUTF(env, message);
}
