#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_config.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Config_nCreate(JNIEnv *env, jclass type) {
    rs2_error *e = NULL;
    rs2_config *rv = rs2_create_config(&e);
    handle_error(env, e);
    return (jlong) rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_config((rs2_config *) handle);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableStream(JNIEnv *env, jclass type_, jlong handle,
                                                           jint type, jint index, jint width,
                                                           jint height, jint format,
                                                           jint framerate) {
    rs2_error *e = NULL;
    rs2_config_enable_stream((rs2_config *) handle, type, index, width, height, format, framerate, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableDeviceFromFile(JNIEnv *env, jclass type,
                                                                   jlong handle,
                                                                   jstring filePath_) {
    const char *filePath = (*env)->GetStringUTFChars(env, filePath_, 0);

    rs2_error *e = NULL;
    rs2_config_enable_device_from_file((rs2_config *) handle, filePath, &e);
    handle_error(env, e);

    (*env)->ReleaseStringUTFChars(env, filePath_, filePath);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableRecordToFile(JNIEnv *env, jclass type,
                                                                 jlong handle, jstring filePath_) {
    const char *filePath = (*env)->GetStringUTFChars(env, filePath_, 0);

    rs2_error *e = NULL;
    rs2_config_enable_record_to_file((rs2_config *) handle, filePath, &e);
    handle_error(env, e);

    (*env)->ReleaseStringUTFChars(env, filePath_, filePath);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDisableStream(JNIEnv *env, jclass type, jlong handle,
                                                            jint streamType) {
    rs2_error *e = NULL;
    rs2_config_disable_stream((rs2_config *) handle, streamType, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableAllStreams(JNIEnv *env, jclass type,
                                                               jlong handle) {
    rs2_error *e = NULL;
    rs2_config_enable_all_stream((rs2_config *) handle, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDisableAllStreams(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error *e = NULL;
    rs2_config_disable_all_streams((rs2_config *) handle, &e);
    handle_error(env, e);
}
