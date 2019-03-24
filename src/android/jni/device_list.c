#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nGetDeviceCount(JNIEnv *env, jclass type,
                                                                 jlong handle) {
    rs2_error *e = NULL;
    auto rv = rs2_get_device_count(handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nCreateDevice(JNIEnv *env, jclass type,
                                                               jlong handle, jint index) {
    rs2_error *e = NULL;
    rs2_device* rv = rs2_create_device(handle, index, &e);
    handle_error(env, e);
    return (jlong)rv;
}

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nContainsDevice(JNIEnv *env, jclass type,
                                                                 jlong handle, jlong deviceHandle) {
    rs2_error *e = NULL;
    auto rv = rs2_device_list_contains(handle, deviceHandle, &e);
    handle_error(env, e);
    return rv > 1;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceList_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_device_list((rs2_device_list *) handle);
}
