#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_RsContext_nCreate(JNIEnv *env, jclass type) {
    rs2_error* e = NULL;
    rs2_context* handle = rs2_create_context(RS2_API_VERSION, &e);
    handle_error(env, e);
    return (jlong) handle;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_RsContext_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_context((rs2_context *) handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_RsContext_nQueryDevices(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_error* e = NULL;
    rs2_device_list* device_list_handle = rs2_query_devices_ex((rs2_context *) handle, RS2_PRODUCT_LINE_ANY_INTEL, &e);
    handle_error(env, e);
    return (jlong) device_list_handle;
}
