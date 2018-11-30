#include <jni.h>
#include <string>
#include <sstream>

#include <cassert>
#include <unordered_map>
#include <vector>
#include <unistd.h>
#include <android/log.h>
#include <Android.h>
#include <thread>
#include "../../../../third_party/librealsense/src/android/usbhost_uvc/usbmanager.h"
#include "../../../../third_party/librealsense/src/android/usbhost_uvc/usbhost.h"
#include "../../../../third_party/librealsense/include/librealsense2/hpp/rs_pipeline.hpp"


UsbManager &usbHost = UsbManager::getInstance();

bool isStreaming = false;

rs2::pipeline_profile profile;

void open_device(usb_device_handle *pDevice, __u8 *outputAddress) {


}

bool running = false;
std::thread t;
rs2::pipeline p;
void *output;
rs2::colorizer colorizer;

void call_from_thread() {
    //colorizer.set_option(RS2_OPTION_HISTOGRAM_EQUALIZATION_ENABLED, 1.f);
    colorizer.set_option(RS2_OPTION_COLOR_SCHEME, 0);
    while (isStreaming) {

        auto frameset = p.wait_for_frames(600000);
        auto depth = frameset.get_depth_frame();
        if (depth.get_data() != nullptr && output != NULL)
            memcpy((unsigned char *) output, depth.get_data(),
                   depth.get_height() * depth.get_stride_in_bytes());
    }
}


extern "C"

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_libusbhost_MainActivity_librsStartStreaming(JNIEnv *env,
                                                                     jobject instance,
                                                                     jobject buffer) {
    rs2::config cfg;
    output = env->GetDirectBufferAddress(buffer);
    cfg.enable_stream(RS2_STREAM_DEPTH, 848, 480);
    profile = p.start(cfg);
    isStreaming = true;
    sleep(2);
    t = std::thread(call_from_thread);
    return true;
}

extern "C"

JNIEXPORT jboolean JNICALL
Java_com_intel_realsense_libusbhost_MainActivity_librsStopStreaming(JNIEnv *env,
                                                                    jobject instance) {

    if (isStreaming) {
        t.join();
        p.stop();
        isStreaming = false;
    }
    return true;
}

std::ostream &operator<<(std::ostream &strm, const UsbDevice &a) {
    const usb_device_descriptor *desc = a.GetDescriptor();
    return strm << a.GetDeviceDescription();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_intel_realsense_libusbhost_RealsenseUsbHostManager_nativeAddUsbDevice(JNIEnv *env,
                                                                               jobject instance,
                                                                               jstring deviceName_,
                                                                               jint fileDescriptor) {
    const char *deviceName = env->GetStringUTFChars(deviceName_, 0);
    usbHost.AddDevice(deviceName, fileDescriptor);
    env->ReleaseStringUTFChars(deviceName_, deviceName);
}