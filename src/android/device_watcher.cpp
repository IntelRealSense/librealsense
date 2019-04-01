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

using namespace std;
using namespace librealsense;
using namespace librealsense::platform;

static std::shared_ptr<device_watcher_usbhost> _instance;

std::shared_ptr<device_watcher_usbhost> device_watcher_usbhost::instance()
{
    if(!_instance)
        _instance = std::make_shared<device_watcher_usbhost>();
    return _instance;
}

void device_watcher_usbhost::notify()
{
    std::lock_guard<std::mutex> lk(_mutex);

    backend_device_group curr;
    curr.uvc_devices = query_uvc_devices();

    if(_callback)
        _callback(_prev_group, curr);
    _prev_group = curr;
}

void device_watcher_usbhost::start(librealsense::platform::device_changed_callback callback)
{
    _callback = callback;
}

void device_watcher_usbhost::stop()
{
    _callback = nullptr;
}

std::vector<platform::uvc_device_info> device_watcher_usbhost::query_uvc_devices() {
    std::vector<platform::uvc_device_info> rv;
    auto usb_devices = platform::usb_enumerator::query_devices();
    for (auto&& dev : usb_devices) {
        for (auto&& in : dev->get_control_interfaces_numbers()) {
            auto inter = dev->get_interface(in);
            stringstream uid;
            uid << dev->get_info().id << "-" << (int)(inter->get_number());
            platform::uvc_device_info device_info;
            device_info.vid = dev->get_info().vid;
            device_info.pid = dev->get_info().pid;
            device_info.mi = inter->get_number();
            device_info.id = uid.str();//TODO_MK
            device_info.unique_id = dev->get_info().id;
            device_info.device_path = dev->get_info().id;
            device_info.conn_spec = dev->get_info().conn_spec;
            LOG_INFO("Found UVC Device vid: " << std::string(device_info).c_str());
            rv.push_back(device_info);
        }
    }
    return rv;
}
