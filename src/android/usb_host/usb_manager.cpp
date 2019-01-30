#include "usb_manager.h"
#include <vector>
#include "usb_device.h"

using namespace std;
using namespace librealsense::usb_host;

static std::vector<std::shared_ptr<usb_device>> _devices;

std::vector<std::shared_ptr<usb_device>> usb_manager::get_device_list()
{
    return _devices;
}

std::shared_ptr<usb_device> usb_manager::get_usb_device_from_handle(usb_device_handle* handle){
   for(auto& d : _devices){
       if(d->GetHandle() == handle)
           return d;
   }
   return nullptr;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_UsbHub_nAddUsbDevice(JNIEnv *env, jclass type,
                                                           jstring deviceName_,
                                                           jint fileDescriptor) {
    const char *deviceName = env->GetStringUTFChars(deviceName_, 0);

    auto handle = usb_device_new(deviceName, fileDescriptor);
    usb_device_reset(handle);
    if (handle != NULL) {
        auto device = std::make_shared<usb_device>(handle);
        _devices.push_back(device);
    }

    env->ReleaseStringUTFChars(deviceName_, deviceName);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_UsbHub_nRemoveUsbDevice(JNIEnv *env, jclass type,
                                                              jint fileDescriptor) {
    _devices.erase(std::remove_if(_devices.begin(), _devices.end(), [fileDescriptor](std::shared_ptr<usb_device> d)
    {
        if(fileDescriptor == d->GetHandle()->fd)
            return true;
    }), _devices.end());
}
