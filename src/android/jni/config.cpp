// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_config.h"

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Config_nCreate(JNIEnv *env, jclass type) {
    rs2_error *e = NULL;
    rs2_config *rv = rs2_create_config(&e);
    handle_error(env, e);
    return (jlong) rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_config((rs2_config *) handle);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableStream(JNIEnv *env, jclass type_, jlong handle,
                                                           jint type, jint index, jint width,
                                                           jint height, jint format,
                                                           jint framerate) {
    rs2_error *e = NULL;
    rs2_config_enable_stream(reinterpret_cast<rs2_config *>(handle), static_cast<rs2_stream>(type), index, width, height,
                             static_cast<rs2_format>(format), framerate, &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableDeviceFromFile(JNIEnv *env, jclass type,
                                                                   jlong handle,
                                                                   jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    rs2_error *e = NULL;
    rs2_config_enable_device_from_file(reinterpret_cast<rs2_config *>(handle), filePath, &e);
    handle_error(env, e);

    env->ReleaseStringUTFChars(filePath_, filePath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableDevice(JNIEnv *env, jclass type,
                                                                   jlong handle,
                                                                   jstring serial_) {
    const char *serial = env->GetStringUTFChars(serial_, 0);

    rs2_error *e = NULL;
    rs2_config_enable_device(reinterpret_cast<rs2_config *>(handle), serial, &e);
    handle_error(env, e);

    env->ReleaseStringUTFChars(serial_, serial);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableRecordToFile(JNIEnv *env, jclass type,
                                                                 jlong handle, jstring filePath_) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);

    rs2_error *e = NULL;
    rs2_config_enable_record_to_file(reinterpret_cast<rs2_config *>(handle), filePath, &e);
    handle_error(env, e);

    env->ReleaseStringUTFChars(filePath_, filePath);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDisableStream(JNIEnv *env, jclass type, jlong handle,
                                                            jint streamType) {
    rs2_error *e = NULL;
    rs2_config_disable_stream(reinterpret_cast<rs2_config *>(handle),
                              static_cast<rs2_stream>(streamType), &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nEnableAllStreams(JNIEnv *env, jclass type,
                                                               jlong handle) {
    rs2_error *e = NULL;
    rs2_config_enable_all_stream(reinterpret_cast<rs2_config *>(handle), &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Config_nDisableAllStreams(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error *e = NULL;
    rs2_config_disable_all_streams(reinterpret_cast<rs2_config *>(handle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Config_nResolve(JNIEnv *env, jclass type, jlong handle,
                                                      jlong pipelineHandle) {
    rs2_error *e = NULL;
    rs2_pipeline_profile* rv = rs2_config_resolve(reinterpret_cast<rs2_config *>(handle),
                                                  reinterpret_cast<rs2_pipeline *>(pipelineHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Config_nCanResolve(JNIEnv *env, jclass type, jlong handle,
                                                         jlong pipelineHandle) {
    rs2_error *e = NULL;
    auto rv = rs2_config_can_resolve(reinterpret_cast<rs2_config *>(handle),
                                     reinterpret_cast<rs2_pipeline *>(pipelineHandle), &e);
    handle_error(env, e);
    return rv > 0;
}
