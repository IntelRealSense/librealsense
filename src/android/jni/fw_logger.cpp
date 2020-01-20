// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include <memory>

#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../fw-logger/rs-fw-logger.h"

std::shared_ptr<android_fw_logger> g_fw_logger;
#define TAG "rs_fw_log"

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nStartReadingFwLogs(JNIEnv *env, jclass clazz,
        jstring file_path) {
    const char *filePath = env->GetStringUTFChars(file_path, 0);
    g_fw_logger = std::make_shared<android_fw_logger>(filePath);

    env->ReleaseStringUTFChars(file_path, filePath);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FwLogger_nStopReadingFwLogs(JNIEnv *env, jclass clazz) {
    g_fw_logger.reset();
}
