#include <jni.h>
#include <error.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_config.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Config_nCreate(JNIEnv *env, jclass type) {
    int error = 0;
    return rs2_create_config(error);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_config(handle);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableStream(JNIEnv *env, jclass type_, jlong handle,
                                                           jint type, jint index, jint width,
                                                           jint height, jint format,
                                                           jint framerate) {
    rs2_error *e = NULL;
    rs2_config_enable_stream(handle, type, index, width, height, format, framerate, &e);
}
