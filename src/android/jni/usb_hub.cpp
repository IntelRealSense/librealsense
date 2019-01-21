#include <jni.h>
#include "../android_uvc/UsbManager.h"

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_UsbHub_nAddUsbDevice(JNIEnv *env, jclass type,
                                                           jstring deviceName_,
                                                           jint fileDescriptor) {
    const char *deviceName = env->GetStringUTFChars(deviceName_, 0);
    UsbManager::getInstance().AddDevice(deviceName, fileDescriptor);
    env->ReleaseStringUTFChars(deviceName_, deviceName);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_UsbHub_nRemoveUsbDevice(JNIEnv *env, jclass type,
                                                              jint fileDescriptor) {
    UsbManager::getInstance().RemoveDevice(fileDescriptor);
}
