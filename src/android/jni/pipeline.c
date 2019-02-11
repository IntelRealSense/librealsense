#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_pipeline.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nCreate(JNIEnv *env, jclass type,
                                                       jlong context) {
    rs2_error* e = NULL;
    rs2_pipeline* rv = rs2_create_pipeline(context, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStart(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    long rv = rs2_pipeline_start(handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfig(JNIEnv *env, jclass type,
                                                                jlong handle, jlong configHandle) {
    rs2_error *e = NULL;
    long rv = rs2_pipeline_start_with_config((rs2_pipeline *) handle, configHandle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStop(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_stop((rs2_pipeline *) handle, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nDelete(JNIEnv *env, jclass type,
                                                       jlong handle) {
    rs2_delete_pipeline((rs2_pipeline *) handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nWaitForFrames(JNIEnv *env, jclass type,
                                                              jlong handle, jint timeout) {
    rs2_error* e = NULL;
    rs2_frame *rv = rs2_pipeline_wait_for_frames((rs2_pipeline *) handle, timeout, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_PipelineProfile_nDelete(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_delete_pipeline_profile(handle);
}
