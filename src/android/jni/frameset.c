#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nAddRef(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) handle, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nRelease(JNIEnv *env, jclass type,
                                                        jlong handle) {
    rs2_release_frame((rs2_frame *) handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nExtractFrame(JNIEnv *env, jclass type,
                                                             jlong handle, jint index) {
    rs2_error *e = NULL;
    rs2_frame *rv = rs2_extract_frame((rs2_frame *) handle, index, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nFrameCount(JNIEnv *env, jclass type,
                                                           jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_embedded_frames_count((rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}