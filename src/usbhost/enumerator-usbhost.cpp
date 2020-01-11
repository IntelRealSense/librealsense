// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#ifdef RS2_USE_ANDROID_BACKEND

#include <jni.h>
#include "device-usbhost.h"
#include "interface-usbhost.h"
#include "../usb/usb-device.h"
#include "../usb/usb-enumerator.h"
#include "../types.h"
#include "usbhost.h"
#include "../android/device_watcher.h"

#include <string>
#include <regex>
#include <sstream>
#include <mutex>

static std::vector<std::shared_ptr<librealsense::platform::usb_device>> _devices;

namespace librealsense
{
    namespace platform
    {
        std::vector<usb_device_info> usb_enumerator::query_devices_info()
        {
            std::vector<usb_device_info> rv;
            for(auto&& dev : _devices)
            {
                auto d = std::static_pointer_cast<usb_device_usbhost>(dev);
                auto sdi = d->get_subdevices_info();
                rv.insert(rv.end(), sdi.begin(), sdi.end());
            }
            return rv;
        }

        rs_usb_device usb_enumerator::create_usb_device(const usb_device_info &info)
        {
            for(auto&& dev : _devices)
            {
                if(info.id == dev->get_info().id)
                    return dev;
            }
            return nullptr;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceWatcher_nAddUsbDevice(JNIEnv *env, jclass type,
    jstring deviceName_, jint fileDescriptor) {
    LOG_DEBUG("AddUsbDevice, previous device count: " << _devices.size());
    const char *deviceName = env->GetStringUTFChars(deviceName_, 0);
    LOG_DEBUG("AddUsbDevice, adding device: " << deviceName << ", descriptor: " << fileDescriptor);

    auto handle = usb_device_new(deviceName, fileDescriptor);
    env->ReleaseStringUTFChars(deviceName_, deviceName);

    if (handle != NULL) {
        LOG_DEBUG("AddUsbDevice, create");
        auto d = std::make_shared<librealsense::platform::usb_device_usbhost>(handle);
        _devices.push_back(d);
    }

    LOG_DEBUG("AddUsbDevice, notify");

    librealsense::platform::device_watcher_usbhost::instance()->notify();

    LOG_DEBUG("AddUsbDevice, current device count: " << _devices.size());
}

extern "C" JNIEXPORT void JNICALL
Java_com_intel_realsense_librealsense_DeviceWatcher_nRemoveUsbDevice(JNIEnv *env, jclass type,
    jint fileDescriptor) {
    LOG_DEBUG("RemoveUsbDevice, previous device count: " << _devices.size());

    _devices.erase(std::remove_if(_devices.begin(), _devices.end(),
        [fileDescriptor](std::shared_ptr<librealsense::platform::usb_device> dev)
    {
        auto d = std::static_pointer_cast<librealsense::platform::usb_device_usbhost>(dev);
        if(fileDescriptor == d->get_file_descriptor()){
//            d->release();
            LOG_DEBUG("RemoveUsbDevice, removing device: " << d->get_info().unique_id << ", descriptor: " << fileDescriptor);
            return true;
        }
        return false;
    }), _devices.end());

    librealsense::platform::device_watcher_usbhost::instance()->notify();

    LOG_DEBUG("RemoveUsbDevice, current device count: " << _devices.size());
}

#endif