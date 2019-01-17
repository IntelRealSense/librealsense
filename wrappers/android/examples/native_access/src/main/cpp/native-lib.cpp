#include <jni.h>
#include "../include/librealsense2/rs.h"

extern "C" JNIEXPORT jstring JNICALL
Java_com_intel_realsense_native_1access_MainActivity_nGetLibrealsenseVersionFromJNI(JNIEnv *env,
                                                                                    jobject instance) {
    return (*env).NewStringUTF(RS2_API_VERSION_STR);
}
