#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"



JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Device_nSupportsInfo(JNIEnv *env, jclass type, jlong handle,
                                                           jint info) {
    rs2_error *e = NULL;
    auto rv = rs2_supports_device_info(handle, info, &e);
    handle_error(env, e);
    return rv > 0;
}

JNIEXPORT jstring JNICALL
Java_com_intel_realsense_librealsense_Device_nGetInfo(JNIEnv *env, jclass type, jlong handle,
                                                      jint info) {
    rs2_error *e = NULL;
    const char* rv = rs2_get_device_info(handle, info, &e);
    handle_error(env, e);
    return (*env)->NewStringUTF(env, rv);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Device_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_device((rs2_device *) handle);
}
