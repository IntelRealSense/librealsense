// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"
#include "../../../include/librealsense2/h/rs_pipeline.h"

#include "jni_logging.h"
#include "frame_callback.h"
#include "jni_common.h"

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nCreate(JNIEnv *env, jclass type,
                                                       jlong context) {
    rs2_error* e = NULL;
    rs2_pipeline* rv = rs2_create_pipeline(reinterpret_cast<rs2_context *>(context), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStart(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    auto rv = rs2_pipeline_start(reinterpret_cast<rs2_pipeline *>(handle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

static frame_callback_data pdata = {NULL, 0, NULL, NULL};

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithCallback(JNIEnv *env, jclass type, jlong handle, jobject jcb) {
    rs2_error* e = NULL;

    if (rs_jni_callback_init(env, handle, jcb, &pdata) != true) return NULL;

    auto cb = [&](rs2::frame f) {
        rs_jni_cb(f, &pdata);
    };

    auto rv = rs2_pipeline_start_with_callback_cpp(reinterpret_cast<rs2_pipeline *>(handle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfig(JNIEnv *env, jclass type,
                                                                jlong handle, jlong configHandle) {
    rs2_error *e = NULL;
    auto rv = rs2_pipeline_start_with_config(reinterpret_cast<rs2_pipeline *>(handle),
                                             reinterpret_cast<rs2_config *>(configHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStartWithConfigAndCallback(JNIEnv *env, jclass type,
                                                                jlong handle, jlong configHandle, jobject jcb) {
    rs2_error *e = NULL;

    if (rs_jni_callback_init(env, handle, jcb, &pdata) != true) return NULL;

    auto cb = [&](rs2::frame f) {
        rs_jni_cb(f, &pdata);
    };

    auto rv = rs2_pipeline_start_with_config_and_callback_cpp(reinterpret_cast<rs2_pipeline *>(handle),
                                             reinterpret_cast<rs2_config *>(configHandle), new rs2::frame_callback<decltype(cb)>(cb), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nStop(JNIEnv *env, jclass type, jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_stop(reinterpret_cast<rs2_pipeline *>(handle), &e);
    handle_error(env, e);

    rs_jni_cleanup(env, &pdata);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nDelete(JNIEnv *env, jclass type,
                                                       jlong handle) {
    rs2_delete_pipeline(reinterpret_cast<rs2_pipeline *>(handle));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nWaitForFrames(JNIEnv *env, jclass type,
                                                              jlong handle, jint timeout) {
    rs2_error* e = NULL;
    rs2_frame *rv = rs2_pipeline_wait_for_frames(reinterpret_cast<rs2_pipeline *>(handle), timeout, &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlongArray JNICALL
Java_com_intel_realsense_librealsense_Pipeline_nGetActiveStreams(JNIEnv *env, jclass type,
                                                       jlong handle) {
    rs2_error* e = NULL;
    rs2_pipeline_profile* profile = rs2_pipeline_get_active_profile(reinterpret_cast<rs2_pipeline *>(handle), &e);
    handle_error(env, e);

    std::shared_ptr<rs2_stream_profile_list> list(
            rs2_pipeline_profile_get_streams(profile, &e),
            rs2_delete_stream_profiles_list);
    handle_error(env, e);

    jlongArray rv = rs_jni_convert_stream_profiles(env, list);
    return rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_PipelineProfile_nDelete(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_delete_pipeline_profile(reinterpret_cast<rs2_pipeline_profile *>(handle));
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_PipelineProfile_nGetDevice(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_error* e = NULL;
    rs2_device *rv = rs2_pipeline_profile_get_device(reinterpret_cast<rs2_pipeline_profile *>(handle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}
