/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2021 Intel Corporation. All Rights Reserved. */

#ifndef LIBREALSENSE_JNI_LOGGING_H
#define LIBREALSENSE_JNI_LOGGING_H

#include <android/log.h>

#define  LRS_JNI_LOG_TAG    "librs_jni_api"

#define  LRS_JNI_LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LRS_JNI_LOG_TAG,__VA_ARGS__)
#define  LRS_JNI_LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LRS_JNI_LOG_TAG,__VA_ARGS__)
#define  LRS_JNI_LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LRS_JNI_LOG_TAG,__VA_ARGS__)
#define  LRS_JNI_LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LRS_JNI_LOG_TAG,__VA_ARGS__)

#endif
