#include <jni.h>
#include <stdint.h>
#include "arm64_runtime.h"

JNIEXPORT jstring JNICALL
Java_org_ps3translator_prototype_MainActivity_nativeStatus(JNIEnv *env, jclass clazz) {
    (void)clazz;
    const char *message = "Android app + JNI bridge loaded. Native runtime is experimental; no PS3 games run yet.";
    return (*env)->NewStringUTF(env, message);
}
