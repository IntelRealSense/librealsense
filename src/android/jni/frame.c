#include <jni.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nAddRef(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    rs2_frame_add_ref(handle, &e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nRelease(JNIEnv *env, jclass type, jlong handle) {
    rs2_release_frame(handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetStreamProfile(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_stream_profile(handle, &e);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetData(JNIEnv *env, jclass type, jlong handle,
                                                     jbyteArray data_) {
    jsize length = (*env)->GetArrayLength(env, data_);
    rs2_error *error = NULL;
    (*env)->SetByteArrayRegion(env, data_, 0, length, rs2_get_frame_data(handle, &error));
}

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_librealsense_Frame_nIsFrameExtendableTo(JNIEnv *env, jclass type,
                                                                 jlong handle, jint extension) {
    rs2_error *e = NULL;
    return rs2_is_frame_extendable_to(handle, extension, &e);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetWidth(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_width(handle, &e);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetHeight(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_height(handle, &e);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetStride(JNIEnv *env, jclass type,
                                                            jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_stride_in_bytes(handle, &e);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_VideoFrame_nGetBitsPerPixel(JNIEnv *env, jclass type,
                                                                  jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_bits_per_pixel(handle, &e);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetNumber(JNIEnv *env, jclass type, jlong handle) {
    rs2_error *e = NULL;
    return rs2_get_frame_number(handle, &e);
}
