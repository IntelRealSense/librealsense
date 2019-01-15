#include <jni.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nAddRef(JNIEnv *env, jobject instance, jlong handle, jint error) {
    rs2_frame_add_ref(handle, &error);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nRelease(JNIEnv *env, jobject instance, jlong handle) {
    rs2_release_frame(handle);
}

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetStreamProfile(JNIEnv *env, jobject instance,
                                                              jlong handle, jint error) {
    return rs2_get_frame_stream_profile(handle, &error);
}

JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_Frame_nGetData(JNIEnv *env, jobject instance, jlong handle,
                                                     jbyteArray data_) {
    jsize length = (*env)->GetArrayLength(env, data_);
    rs2_error *error = NULL;
    (*env)->SetByteArrayRegion(env, data_, 0, length, rs2_get_frame_data(handle, &error));
//    jbyte *data = (*env)->GetByteArrayElements(env, data_, NULL);
    // TODO
//    (*env)->ReleaseByteArrayElements(env, data_, data, 0);
}