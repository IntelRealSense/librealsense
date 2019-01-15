#include <jni.h>
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_pipeline.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nCreate(JNIEnv *env, jobject instance,
                                                       jlong context) {
    rs2_error* e = NULL;
    rs2_pipeline* handle = rs2_create_pipeline(context, &e);
    return handle;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStart(JNIEnv *env, jobject instance, jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_start(handle, &e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStop(JNIEnv *env, jobject instance, jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_stop(handle, &e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nDelete(JNIEnv *env, jobject instance,
                                                       jlong handle) {

    rs2_delete_pipeline(handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nWaitForFrames(JNIEnv *env, jobject instance,
                                                              jlong handle, jint timeout) {
    rs2_error* e = NULL;
    return rs2_pipeline_wait_for_frames(handle, timeout, &e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfig(JNIEnv *env, jobject instance,
                                                          jlong handle, jlong configHandle) {
    rs2_error *e = NULL;
    rs2_pipeline_start_with_config(handle, configHandle, &e);
}