// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nGetDeviceCount(JNIEnv *env, jclass type,
                                                                 jlong handle) {
    rs2_error *e = NULL;
    auto rv = rs2_get_device_count(reinterpret_cast<const rs2_device_list *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nCreateDevice(JNIEnv *env, jclass type,
                                                               jlong handle, jint index) {
    rs2_error *e = NULL;
    rs2_device* rv = rs2_create_device(reinterpret_cast<const rs2_device_list *>(handle), index, &e);
    handle_error(env, e);
    return (jlong)rv;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nContainsDevice(JNIEnv *env, jclass type,
                                                                 jlong handle, jlong deviceHandle) {
    rs2_error *e = NULL;
    auto rv = rs2_device_list_contains(reinterpret_cast<const rs2_device_list *>(handle),
                                       reinterpret_cast<const rs2_device *>(deviceHandle), &e);
    handle_error(env, e);
    return rv > 0;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_device_list((rs2_device_list *) handle);
}
