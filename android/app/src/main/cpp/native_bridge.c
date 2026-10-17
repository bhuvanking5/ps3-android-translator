#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"
#include "ppc_cpu.h"
#include "ppc_to_arm64.h"
#include "controller_input.h"
#include "guest_elf_loader.h"
#include "guest_memory.h"
#include <stdlib.h>
#include <stdio.h>

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


/*
 * Experimental bounded interpreter path for a small, unencrypted ELF64/PPC64
 * file. It never executes the file as host machine code. This is not a PS3
 * process/runtime and cannot boot PKG/SELF games.
 */
JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeRunSelectedElf(
    JNIEnv *env, jclass clazz, jbyteArray input) {
    (void)clazz;
    if (input == 0) return (*env)->NewStringUTF(env, "No file bytes were supplied.");
    const jsize input_size = (*env)->GetArrayLength(env, input);
    if (input_size < 64 || (uint64_t)input_size > 32u * 1024u * 1024u)
        return (*env)->NewStringUTF(env, "ELF runner rejected the file: size must be 64 bytes to 32 MiB for this prototype.");
    uint8_t *file = (uint8_t *)malloc((size_t)input_size);
    uint8_t *ram = (uint8_t *)calloc(1u, 64u * 1024u * 1024u);
    if (file == 0 || ram == 0) {
        free(file);
        free(ram);
        return (*env)->NewStringUTF(env, "Not enough memory for the bounded ELF test. No game was launched.");
    }
    (*env)->GetByteArrayRegion(env, input, 0, input_size, (jbyte *)file);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        free(file);
        free(ram);
        return (*env)->NewStringUTF(env, "Android could not read the selected file bytes.");
    }

    ps3_guest_memory memory;
    ps3_guest_memory_init(&memory);
    uint32_t entry = 0u;
    const ps3_guest_elf_status load_status = ps3_guest_elf_load(
        file, (size_t)input_size, &memory, ram, 0u,
        64u * 1024u * 1024u, &entry);
    if (load_status != PS3_GUEST_ELF_OK) {
        free(file);
        free(ram);
        if (load_status == PS3_GUEST_ELF_BAD_FORMAT)
            return (*env)->NewStringUTF(env, "ELF loader rejected this file's structure or entry point. PKG/SELF and real PS3 startup are not supported.");
        if (load_status == PS3_GUEST_ELF_OUT_OF_BOUNDS)
            return (*env)->NewStringUTF(env, "ELF uses addresses outside this prototype's 64 MiB guest RAM window. No game was launched.");
        if (load_status == PS3_GUEST_ELF_MEMORY_MAP_ERROR)
            return (*env)->NewStringUTF(env, "ELF memory map could not be created safely. No game was launched.");
        return (*env)->NewStringUTF(env, "This file is not a supported unencrypted big-endian ELF64 PowerPC64 executable. PKG/SELF games cannot launch yet.");
    }

    /*
     * Provide a bounded, writable initial stack for simple static ELF fixtures.
     * This is not the full PS3 ABI: argc/argv, auxv, TLS, imports and syscalls
     * are still absent. Keep the stack in a reserved top-of-RAM window and
     * refuse to overlap any loaded PT_LOAD segment.
     */
    const uint32_t stack_size = 1024u * 1024u;
    const uint32_t stack_base = 63u * 1024u * 1024u;
    const uint32_t stack_top = stack_base + stack_size;
    if (!ps3_guest_memory_map(&memory, stack_base, stack_size,
            ram + stack_base, PS3_GUEST_MEMORY_READ | PS3_GUEST_MEMORY_WRITE)) {
        free(file);
        free(ram);
        return (*env)->NewStringUTF(env,
            "ELF segments overlap the reserved 1 MiB startup stack window. "
            "This prototype cannot construct a safe initial guest stack.");
    }

    uint64_t gpr[32] = {0};
    gpr[1] = (uint64_t)(stack_top & ~15u); /* PPC ABI stack pointer, 16-byte aligned */
    size_t steps = 0u;
    const ppc_cpu_status run_status = ppc_cpu_run_guest(
        &memory, entry, gpr, 1024u, &steps);
    char message[320];
    if (run_status == PPC_CPU_UNSUPPORTED_INSTRUCTION) {
        (void)snprintf(message, sizeof(message),
            "ELF loaded at 0x%08x; interpreter ran %zu step(s), then hit an unsupported PowerPC instruction. r3=0x%llx. This is a launch-pipeline test, not a booted PS3 game.",
            entry, steps, (unsigned long long)gpr[3]);
    } else if (run_status == PPC_CPU_MEMORY_ACCESS_ERROR) {
        (void)snprintf(message, sizeof(message),
            "ELF loaded at 0x%08x; interpreter stopped after %zu step(s) on an unmapped/invalid guest memory access. PS3 process setup is not implemented.",
            entry, steps);
    } else if (run_status == PPC_CPU_GUEST_EXIT) {
        (void)snprintf(message, sizeof(message),
            "ELF loaded at 0x%08x; guest returned cleanly after %zu step(s) (r3=0x%llx, r4=0x%llx, r5=0x%llx). This is a synthetic ELF test, not a booted PS3 game.",
            entry, steps, (unsigned long long)gpr[3],
            (unsigned long long)gpr[4], (unsigned long long)gpr[5]);
    } else if (run_status == PPC_CPU_STEP_LIMIT) {
        (void)snprintf(message, sizeof(message),
            "ELF loaded at 0x%08x; reached the 1024-step safety limit (r3=0x%llx, r4=0x%llx, r5=0x%llx). This is only bounded guest execution, not a booted PS3 game.",
            entry, (unsigned long long)gpr[3],
            (unsigned long long)gpr[4], (unsigned long long)gpr[5]);
    } else {
        (void)snprintf(message, sizeof(message),
            "ELF runner stopped with status %d after %zu step(s). PS3 game compatibility is not established.",
            (int)run_status, steps);
    }
    free(file);
    free(ram);
    return (*env)->NewStringUTF(env, message);
}


/*
 * Built-in integration fixture: synthesize a tiny ELF64/PPC64 executable and
 * pass it through the same JNI ELF loader/runner used for selected raw ELF
 * files. It is a pipeline test, not a PS3 game.
 */
JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeRunBuiltinElfTest(
    JNIEnv *env, jclass clazz) {
    static const jbyte fixture[] = {
        0x7f, 'E', 'L', 'F', 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 2, 0, 21, 0, 0, 0, 1,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 64,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0,
        0, 64, 0, 56, 0, 1, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 1, 0, 0, 0, 5,
        0, 0, 0, 0, 0, 0, 0, 120,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 1, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 16,
        0, 0, 0, 0, 0, 0, 0, 16,
        0, 0, 0, 0, 0, 0, 0, 4,
        0x38, 0x60, 0x00, 0x07, /* addi r3,0,7 */
        0x38, 0x80, 0x00, 0x05, /* addi r4,0,5 */
        0x7c, 0xa3, 0x22, 0x14, /* add r5,r3,r4 => 12 */
        0x4e, 0x80, 0x00, 0x20  /* blr; prototype top-level clean return */
    };
    jbyteArray bytes = (*env)->NewByteArray(env, (jsize)sizeof(fixture));
    if (bytes == 0) {
        if ((*env)->ExceptionCheck(env)) (*env)->ExceptionClear(env);
        return (*env)->NewStringUTF(env, "Could not allocate the built-in ELF fixture.");
    }
    (*env)->SetByteArrayRegion(env, bytes, 0, (jsize)sizeof(fixture), fixture);
    if ((*env)->ExceptionCheck(env)) {
        (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, bytes);
        return (*env)->NewStringUTF(env, "Could not prepare the built-in ELF fixture.");
    }
    jstring result = Java_org_ps3translator_prototype_MainActivity_nativeRunSelectedElf(
        env, clazz, bytes);
    (*env)->DeleteLocalRef(env, bytes);
    return result;
}

/*
 * Publish virtual-controller input into a native mailbox. A future emulator
 * input loop can read this through ps3_controller_input_read(); this function
 * alone does not make the prototype emulate a PS3 controller or game.
 */
JNIEXPORT void JNICALL
Java_org_ps3translator_prototype_ControllerInputState_nativePublish(
    JNIEnv *env, jclass clazz, jint pressed_mask,
    jfloat left_x, jfloat left_y, jfloat right_x, jfloat right_y) {
    (void)env;
    (void)clazz;
    ps3_controller_input_publish((uint32_t)pressed_mask,
        (float)left_x, (float)left_y, (float)right_x, (float)right_y);
}
