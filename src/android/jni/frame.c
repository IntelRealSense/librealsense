#include <jni.h>
#include "error.h"
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nAddRef(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref((rs2_frame *) handle, &e);
    handle_error(env, e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_release_frame((rs2_frame *) handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetStreamProfile(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    const rs2_stream_profile *rv = rs2_get_frame_stream_profile((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetData(JNIEnv *env, jclass type, jlong handle,
                                                     jbyteArray data_) {
    jsize length = (*env)->GetArrayLength(env, data_);
    rs2_error *e = NULL;
    (*env)->SetByteArrayRegion(env, data_, 0, length, rs2_get_frame_data((const rs2_frame *) handle, &e));
    handle_error(env, e);
}

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Frame_nIsFrameExtendableTo(JNIEnv *env, jclass type,
                                                                 jlong handle, jint extension) {
    rs2_error *e = NULL;
    int rv = rs2_is_frame_extendable_to(handle, extension, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetWidth(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_width(handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetHeight(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_height(handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetStride(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_stride_in_bytes((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetBitsPerPixel(JNIEnv *env, jclass type,
                                                                  jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_bits_per_pixel((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetNumber(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    unsigned long long rv = rs2_get_frame_number((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jfloat JNICALL
Java_com_intel_realsense_librealsense_DepthFrame_nGetDistance(JNIEnv *env, jclass type,
                                                              jlong handle, jint x, jint y) {
    rs2_error *e = NULL;
    float rv = rs2_depth_frame_get_distance((const rs2_frame *) handle, x, y, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Points_nGetCount(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_points_count((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jdouble JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetTimestamp(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    double rv = rs2_get_frame_timestamp((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetTimestampDomain(JNIEnv *env, jclass type,
                                                                jlong handle) {
    rs2_error *e = NULL;
    int rv = rs2_get_frame_timestamp_domain((const rs2_frame *) handle, &e);
    handle_error(env, e);
    return rv;
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetMetadata(JNIEnv *env, jclass type, jlong handle,
                                                         jint metadata_type) {
    rs2_error *e = NULL;
    long rv = rs2_get_frame_metadata((const rs2_frame *) handle, metadata_type, &e);
    handle_error(env, e);
    return rv;
}
