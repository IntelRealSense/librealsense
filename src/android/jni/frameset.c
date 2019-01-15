#include <jni.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nAddRef(JNIEnv *env, jobject instance,
                                                       jlong handle,
                                                       jint error) {
    rs2_frame_add_ref(handle, &error);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nRelease(JNIEnv *env, jobject instance,
                                                        jlong handle) {
    rs2_release_frame(handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nExtractFrame(JNIEnv *env, jobject instance,
                                                             jlong handle, jint index, jint error) {
    return rs2_extract_frame(handle, index, &error);
}

JNIEXPORT jint JNICALL
Java_com_intel_realsense_librealsense_FrameSet_nFrameCount(JNIEnv *env, jobject instance,
                                                           jlong handle, jint error) {
    return rs2_embedded_frames_count(handle, &error);
}