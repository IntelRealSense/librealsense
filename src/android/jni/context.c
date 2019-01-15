#include <jni.h>
#include "../../../include/librealsense2/rs.h"

JNIEXPORT jlong JNICALL
Java_com_intel_realsense_librealsense_RsContext_nCreate(JNIEnv *env, jobject instance) {
    rs2_error* e = NULL;
    rs2_context* handle = rs2_create_context(RS2_API_VERSION, &e);
    return handle;
}

//JNIEXPORT jstring JNICALL
//Java_com_intel_realsense_librealsense_LrsClass_nGetVersion(JNIEnv *env, jobject instance) {
//    return (*env)->NewStringUTF(env, RS2_API_VERSION_STR);
//}