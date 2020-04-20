#include <jni.h>
#include "librealsense2/rs.hpp"

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nGetLibrealsenseVersionFromJNI(JNIEnv *env, jclass type) {
    return (*env).NewStringUTF(RS2_API_VERSION_STR);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nGetCamerasCountFromJNI(JNIEnv *env, jclass type) {
    rs2::context ctx;
    return ctx.query_devices().size();;
}