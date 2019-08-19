// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "device_watcher.h"
#include <vector>
#include <sstream>
#include <jni.h>
#include "../usb/usb-device.h"
#include "../usbhost/device-usbhost.h"
#include "../backend.h"
#include "../usb/usb-enumerator.h"
#include "../hid/hid-device.h"

using namespace std;
using namespace librealsense;
using namespace librealsense::platform;

std::shared_ptr<device_watcher_usbhost> device_watcher_usbhost::instance()
{
    static std::shared_ptr<device_watcher_usbhost> instance = std::make_shared<device_watcher_usbhost>();
    return instance;
}

void device_watcher_usbhost::notify()
{
    backend_device_group curr;
    backend_device_group prev;
    librealsense::platform::device_changed_callback callback;

    curr.uvc_devices = query_uvc_devices();
    curr.hid_devices = query_hid_devices_info();

    {
        std::lock_guard<std::mutex> lk(_mutex);
        prev = _prev_group;
        _prev_group = curr;
        callback = _callback;
    }

    if(callback)
        callback(prev, curr);
}

void device_watcher_usbhost::start(librealsense::platform::device_changed_callback callback)
{
    std::lock_guard<std::mutex> lk(_mutex);
    _callback = callback;
}

void device_watcher_usbhost::stop()
{
    std::lock_guard<std::mutex> lk(_mutex);
    _callback = nullptr;
}

std::vector<platform::uvc_device_info> device_watcher_usbhost::query_uvc_devices() {
    std::vector<platform::uvc_device_info> rv;
    auto usb_devices = platform::usb_enumerator::query_devices_info();
    for (auto&& info : usb_devices) {
        if(info.cls != RS2_USB_CLASS_VIDEO)
            continue;
        platform::uvc_device_info device_info;
        device_info.vid = info.vid;
        device_info.pid = info.pid;
        device_info.mi = info.mi;
        device_info.unique_id = info.unique_id;
        device_info.device_path = info.unique_id;//the device unique_id is the USB port
        device_info.conn_spec = info.conn_spec;
        LOG_INFO("Found UVC device: " << std::string(device_info).c_str());
        rv.push_back(device_info);
    }
    return rv;
}
