// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameQueue_nCreate(JNIEnv *env, jclass type, jint capacity) {
    rs2_error *e = NULL;
    rs2_frame_queue *rv = rs2_create_frame_queue(capacity, &e);
    handle_error(env, e);
    return (jlong) rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameQueue_nDelete(JNIEnv *env, jclass type, jlong handle) {
    rs2_delete_frame_queue((rs2_frame_queue *) handle);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameQueue_nPollForFrame(JNIEnv *env, jclass type,
                                                               jlong handle) {
    rs2_frame *output_frame = NULL;
    rs2_error *e = NULL;
    int rv = rs2_poll_for_frame((rs2_frame_queue *) handle, &output_frame, &e);
    handle_error(env, e);
    return (jlong) (rv ? output_frame : 0);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameQueue_nWaitForFrames(JNIEnv *env, jclass type,
                                                                jlong handle, jint timeout) {
    rs2_error *e = NULL;
    rs2_frame *rv = rs2_wait_for_frame((rs2_frame_queue *) handle, (unsigned int) timeout, &e);
    handle_error(env, e);
    return (jlong) rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameQueue_nEnqueue(JNIEnv *env, jclass type, jlong handle,
                                                          jlong frameHandle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) frameHandle, &e);
    handle_error(env, e);
    rs2_enqueue_frame((rs2_frame *) frameHandle, (void *) handle);
}
