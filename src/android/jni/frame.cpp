// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.

#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nAddRef(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) handle, &e);
    handle_error(env, e);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_release_frame((rs2_frame *) handle);
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetStreamProfile(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    const rs2_stream_profile *rv = rs2_get_frame_stream_profile(
            reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return (jlong) rv;
}

extern "C"
JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetDataSize(JNIEnv *env, jclass type, jlong handle) {

    rs2_error *e = NULL;
    auto rv = rs2_get_frame_data_size(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return (jint)rv;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetData(JNIEnv *env, jclass type, jlong handle,
                                                     jbyteArray data_) {
    jsize length = env->GetArrayLength(data_);
    rs2_error *e = NULL;
    env->SetByteArrayRegion(data_, 0, length, static_cast<const jbyte *>(rs2_get_frame_data(
                reinterpret_cast<const rs2_frame *>(handle), &e)));
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Points_nGetData(JNIEnv *env, jclass type, jlong handle,
                                                      jfloatArray data_) {
    jsize length = env->GetArrayLength(data_);
    rs2_error *e = NULL;
    env->SetFloatArrayRegion(data_, 0, length, static_cast<const jfloat *>(rs2_get_frame_data(
            reinterpret_cast<const rs2_frame *>(handle), &e)));
    handle_error(env, e);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Points_nGetTextureCoordinates(JNIEnv *env, jclass type,
                                                                    jlong handle,
                                                                    jfloatArray data_) {
    jsize length = env->GetArrayLength(data_);
    rs2_error *e = NULL;
    env->SetFloatArrayRegion(data_, 0, length, reinterpret_cast<const jfloat *>(rs2_get_frame_texture_coordinates(
            reinterpret_cast<const rs2_frame *>(handle), &e)));
    handle_error(env, e);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Frame_nIsFrameExtendableTo(JNIEnv *env, jclass type,
                                                                 jlong handle, jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_is_frame_extendable_to(reinterpret_cast<const rs2_frame *>(handle),
                                        static_cast<rs2_extension>(extension), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetWidth(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_width(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetHeight(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_height(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetStride(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_stride_in_bytes(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetBitsPerPixel(JNIEnv *env, jclass type,
                                                                  jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_bits_per_pixel(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetNumber(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    unsigned long long rv = rs2_get_frame_number(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_DepthFrame_nGetDistance(JNIEnv *env, jclass type,
                                                              jlong handle, jint x, jint y) {
    rs2_error *e = NULL;
    float rv = rs2_depth_frame_get_distance(reinterpret_cast<const rs2_frame *>(handle), x, y, &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_DepthFrame_nGetUnits( JNIEnv *env, jclass type, jlong handle ) {
    rs2_error *e = NULL;
    float rv = rs2_depth_frame_get_units( reinterpret_cast<const rs2_frame *>(handle), &e );
    handle_error( env, e );
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Points_nGetCount(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_points_count(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Points_nExportToPly(JNIEnv *env, jclass type, jlong handle,
                                                          jstring filePath_, jlong textureHandle) {
    const char *filePath = env->GetStringUTFChars(filePath_, 0);
    rs2_error *e = NULL;
    rs2_export_to_ply(reinterpret_cast<const rs2_frame *>(handle), filePath,
                      reinterpret_cast<rs2_frame *>(textureHandle), &e);
    handle_error(env, e);
    env->ReleaseStringUTFChars(filePath_, filePath);
}

extern "C" JNIEXPORT jdouble JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetTimestamp(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    double rv = rs2_get_frame_timestamp(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetTimestampDomain(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_timestamp_domain(reinterpret_cast<const rs2_frame *>(handle), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetMetadata(JNIEnv *env, jclass type, jlong handle,
                                                         jint metadata_type) {
    rs2_error *e = NULL;
    long rv = rs2_get_frame_metadata(reinterpret_cast<const rs2_frame *>(handle),
                                     static_cast<rs2_frame_metadata_value>(metadata_type), &e);
    handle_error(env, e);
    return rv;
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Frame_nSupportsMetadata(JNIEnv *env, jclass type, jlong handle,
                                                         jint metadata_type) {
    rs2_error *e = NULL;
    int rv = rs2_supports_frame_metadata(reinterpret_cast<const rs2_frame *>(handle),
                                     static_cast<rs2_frame_metadata_value>(metadata_type), &e);
    handle_error(env, e);
    return rv > 0;
}
