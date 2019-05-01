// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"

#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_ProcessingBlock_nDelete(JNIEnv *env, jclass type,
                                                              jlong handle) {
    rs2_delete_processing_block(reinterpret_cast<rs2_processing_block *>(handle));
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_ProcessingBlock_nInvoke(JNIEnv *env, jclass type,
                                                              jlong handle, jlong frameHandle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref(reinterpret_cast<rs2_frame *>(frameHandle), &e);
    handle_error(env, e);
    rs2_process_frame(reinterpret_cast<rs2_processing_block *>(handle),
                      reinterpret_cast<rs2_frame *>(frameHandle), &e);
    handle_error(env, e);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Align_nCreate(JNIEnv *env, jclass type, jlong queueHandle,
                                                    jint alignTo) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_align(static_cast<rs2_stream>(alignTo), &e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Colorizer_nCreate(JNIEnv *env, jclass type, jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_colorizer(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_DecimationFilter_nCreate(JNIEnv *env, jclass type,
                                                         jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_decimation_filter_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_DisparityTransformFilter_nCreate(JNIEnv *env, jclass type,
                                                                 jlong queueHandle,
                                                                 jboolean transformToDisparity) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_disparity_transform_block(transformToDisparity, &e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_HoleFillingFilter_nCreate(JNIEnv *env, jclass type,
                                                          jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_hole_filling_filter_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Pointcloud_nCreate(JNIEnv *env, jclass type,
                                                         jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_pointcloud(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_SpatialFilter_nCreate(JNIEnv *env, jclass type, jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_spatial_filter_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_TemporalFilter_nCreate(JNIEnv *env, jclass type,
                                                       jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_temporal_filter_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_ThresholdFilter_nCreate(JNIEnv *env, jclass type,
                                                        jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_threshold(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_ZeroOrderInvalidationFilter_nCreate(JNIEnv *env, jclass type,
                                                                    jlong queueHandle) {
    rs2_error *e = NULL;
    rs2_processing_block *rv = rs2_create_zero_order_invalidation_block(&e);
    handle_error(env, e);
    rs2_start_processing_queue(rv, reinterpret_cast<rs2_frame_queue *>(queueHandle), &e);
    handle_error(env, e);
    return reinterpret_cast<jlong>(rv);
}
