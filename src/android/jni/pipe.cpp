#include <jni.h>
#include "../../../include/librealsense2/rs.hpp"

rs2::pipeline* pipe;
rs2::colorizer col;

extern "C" JNIEXPORT void JNICALL Java_com_intel_realsense_librealsense_Pipe_nStart(JNIEnv *env, jobject instance, jint width, jint height) {
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_RGBA8);
    cfg.enable_stream(RS2_STREAM_DEPTH, width, height);
    pipe = new rs2::pipeline();
    pipe->start(cfg);
}
extern "C" JNIEXPORT void JNICALL Java_com_intel_realsense_librealsense_Pipe_nStop(JNIEnv *env, jobject instance) {
    pipe->stop();
}

extern "C" JNIEXPORT void JNICALL Java_com_intel_realsense_librealsense_Pipe_nWaitForFrames(JNIEnv *env, jobject instance, jobject colorBuffer, jobject depthBuffer) {
    rs2::frameset data = pipe->wait_for_frames().apply_filter(col);
    data.foreach([&](const rs2::frame& f)
                 {
                     auto vf = f.as<rs2::video_frame>();
                     auto type = f.get_profile().stream_type();
                     switch (type)
                     {
                         case RS2_STREAM_COLOR:
                             memcpy((unsigned char *)env->GetDirectBufferAddress(colorBuffer), vf.get_data(), vf.get_height() * vf.get_stride_in_bytes()); break;
                         case RS2_STREAM_DEPTH:
                             if(vf.get_bytes_per_pixel() == 4)
                                memcpy((unsigned char *)env->GetDirectBufferAddress(depthBuffer), vf.get_data(), vf.get_height() * vf.get_stride_in_bytes());
                             break;
                         default:
                             return;
                     }
                 });
}