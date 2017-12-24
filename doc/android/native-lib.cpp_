#include <jni.h>
#include <android/native_window_jni.h>
#include <string>
#include <cstring>
#include <thread>
#include "librealsense/include/librealsense2/rs.hpp"
#include "librealsense/include/librealsense2/hpp/rs_context.hpp"
#include "librealsense/include/librealsense2/hpp/rs_pipeline.hpp"
#include "librealsense/include/librealsense2/hpp/rs_internal.hpp"


static rs2::context ctx;
static std::atomic<bool> streaming(false);
static std::thread frame_thread;

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_startStreaming(JNIEnv *env, jobject jobj, jstring className) {
    try {
        if (streaming)
            return;

        const char* nativeClassName = env->GetStringUTFChars(className, 0);
        jclass handlerClass = env->FindClass(nativeClassName);
        env->ReleaseStringUTFChars(className, nativeClassName);
        if (handlerClass == NULL) {
            throw std::runtime_error("FindClass(...) failed!");
        }

        jmethodID onFrame = env->GetStaticMethodID(handlerClass, "onFrame",
                                                   "(Ljava/lang/Object;II)V");
        if (onFrame == NULL) {
            throw std::runtime_error("GetStaticMethodID(...) failed!");
        }

        auto list = ctx.query_devices();
        if (0 == list.size()) {
            throw std::runtime_error("No RealSense devices where found!");
        }


        JavaVM* vm;
        env->GetJavaVM(&vm);
        frame_thread = std::thread([vm, handlerClass, onFrame](){
            static const rs2_format format = RS2_FORMAT_Z16;
            static const rs2_stream stream = RS2_STREAM_DEPTH;
            static const int width = 480, height = 270, bpp = 3, fps = 30;
            static const int rgb_frame_size = width * height * bpp;
            JNIEnv* env;
            vm->AttachCurrentThread(&env, NULL);
            jbyteArray arr = env->NewByteArray(rgb_frame_size);

            try {
                rs2::pipeline pipe;
                rs2::config config;
                rs2::pipeline_profile profile;
                rs2::colorizer colorizer;

                config.enable_stream(stream, width, height, format, fps);
                profile = pipe.start(config);

                streaming = true;
                while (streaming) {
                    rs2::frameset data = pipe.wait_for_frames();
                    auto rgb_vid_frame = colorizer.colorize(data);

                    env->SetByteArrayRegion(arr, 0, rgb_frame_size,
                                            reinterpret_cast<const jbyte *>(rgb_vid_frame.get_data()));

                    env->CallStaticVoidMethod(handlerClass, onFrame, arr, width,
                                              height);
                }
                pipe.stop();
            }
            catch (const std::exception& ex)
            {}

            env->DeleteLocalRef(arr);
            vm->DetachCurrentThread();
        });
    }
    catch (const std::exception& ex)
    {
        jclass jcls = env->FindClass("java/lang/Exception");
        env->ThrowNew(jcls, ex.what());
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_example_realsense_1app_MainActivity_stopStreaming(JNIEnv *env, jobject instance) {
    try {
        if (!streaming)
            return;

            streaming = false;
            if (frame_thread.joinable())
                frame_thread.join();

    }
    catch (const std::exception& ex)
    {
        jclass jcls = env->FindClass("java/lang/Exception");
        env->ThrowNew(jcls, ex.what());
    }
}
