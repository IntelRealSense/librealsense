// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nAddRef(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) handle, &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nRelease(JNIEnv *env, jclass type,
                                                        jlong handle) {
    rs2_release_frame(reinterpret_cast<rs2_frame *>(handle));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nExtractFrame(JNIEnv *env, jclass type,
                                                             jlong handle, jint index) {
    rs2_error *e = NULL;
    rs2_frame *rv = rs2_extract_frame(reinterpret_cast<rs2_frame *>(handle), index, &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nFrameCount(JNIEnv *env, jclass type,
                                                           jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_embedded_frames_count(reinterpret_cast<rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}