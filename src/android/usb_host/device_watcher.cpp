// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "device_watcher.h"
#include <vector>
#include <jni.h>
#include "usb_device.h"
#include "../../backend.h"

using namespace std;
using namespace librealsense;
using namespace librealsense::usb_host;

static std::vector<std::shared_ptr<device>> _devices;
static librealsense::platform::device_changed_callback _callback;

std::vector<std::shared_ptr<device>> device_watcher::get_device_list()
{
    return _devices;
}

void device_watcher::start(librealsense::platform::device_changed_callback callback)
{
    _callback = callback;
}

void device_watcher::stop()
{

}

std::vector<platform::uvc_device_info> device_watcher::query_uvc_devices() {
    std::vector<platform::uvc_device_info> devices;
    for (auto dev : _devices) {
        for (int ia = 0; ia < dev->get_interfaces_associations_count(); ia++) {
            auto iad = dev->get_interface_association(ia);
            if (iad.get_descriptor()->bFunctionClass == USB_CLASS_VIDEO) {
                platform::uvc_device_info device_info;
                device_info.vid = dev->get_vid();
                device_info.pid = dev->get_pid();
                device_info.mi = iad.get_mi();
                device_info.unique_id = dev->get_file_descriptor();
                device_info.device_path = dev->get_name();
                device_info.conn_spec = platform::usb_spec(dev->get_conn_spec());
                LOG_INFO("Found UVC Device vid: " << std::string(device_info).c_str());
                devices.push_back(device_info);
            }
        }
    }
    return devices;
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceWatcher_nAddUsbDevice(JNIEnv *env, jclass type,
                                                           jstring deviceName_,
                                                           jint fileDescriptor) {
    platform::backend_device_group prev;
    prev.uvc_devices = device_watcher::query_uvc_devices();
    const char *deviceName = env->GetStringUTFChars(deviceName_, 0);
    auto handle = usb_device_new(deviceName, fileDescriptor);
    env->ReleaseStringUTFChars(deviceName_, deviceName);

    if (handle != NULL) {
        auto d = std::make_shared<device>(handle);
        _devices.push_back(d);
    }

    platform::backend_device_group curr;
    curr.uvc_devices = device_watcher::query_uvc_devices();

    if(_callback)
        _callback(prev, curr);
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceWatcher_nRemoveUsbDevice(JNIEnv *env, jclass type,
                                                              jint fileDescriptor) {
    platform::backend_device_group prev;
    prev.uvc_devices = device_watcher::query_uvc_devices();

    _devices.erase(std::remove_if(_devices.begin(), _devices.end(), [fileDescriptor](std::shared_ptr<device> d)
    {
        if(fileDescriptor == d->get_file_descriptor())
            return true;
    }), _devices.end());

    platform::backend_device_group curr;
    curr.uvc_devices = device_watcher::query_uvc_devices();

    _callback(prev, curr);
}
