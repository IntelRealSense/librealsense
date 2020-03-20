#include <jni.h>
#include <android/log.h>

#include "librealsense2/rs.hpp"

#define TAG "native_example"

#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR,    TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO,     TAG, __VA_ARGS__)

extern "C" JNIEXPORT jstring JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nGetLibrealsenseVersionFromJNI(JNIEnv *env, jclass type) {
    return (*env).NewStringUTF(RS2_API_VERSION_STR);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nGetCamerasCountFromJNI(JNIEnv *env, jclass type) {
    rs2::context ctx;
    return ctx.query_devices().size();
}

extern "C" JNIEXPORT jint JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nGetSensorsCountFromJNI(JNIEnv *env, jclass type) {
    rs2::context ctx;
    return ctx.query_all_sensors().size();
}

static rs2::pipeline pipe;
static rs2::config cfg;

extern "C" JNIEXPORT void JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nStreamPoseData(JNIEnv *env, jclass type) {
    rs2::context ctx;
    
    if(ctx.query_devices().size() == 0) {
        LOGE("No pose devices available");
        return;
    }

    cfg.enable_stream(RS2_STREAM_POSE, RS2_FORMAT_6DOF);

    // Start pipeline with chosen configuration
    pipe.start(cfg);
}

extern "C" JNIEXPORT void JNICALL
Java_com_example_realsense_1native_1example_MainActivity_nStopStream(JNIEnv *env, jclass type) {
    // occasionally stop requests arrive before library
    // had time to update device list and fail on attempt to stop the stream
    try {
        LOGI("Stopping stream");
        pipe.stop();
        cfg.disable_all_streams();
    }
    catch(...) {
        LOGE("Unable to stop stream");
    }
}
