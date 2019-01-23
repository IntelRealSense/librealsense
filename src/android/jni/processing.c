#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_ProcessingBlock_nDelete(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_delete_processing_block(handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Colorizer_nCreate(JNIEnv *env, jclass type, jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_colorizer(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, queueHandle, &e);
    handle_error(env, e);
    return (jlong) rv;
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Decimation_nCreate(JNIEnv *env, jclass type,
                                                         jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_decimation_filter_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, queueHandle, &e);
    handle_error(env, e);
    return (jlong) rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_ProcessingBlock_nInvoke(JNIEnv *env, jclass type,
                                                              jlong handle, jlong frameHandle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) frameHandle, &e);
    handle_error(env, e);
    rs2_process_frame((rs2_processing_block *) handle, (rs2_frame *) frameHandle, &e);
    handle_error(env, e);
}
