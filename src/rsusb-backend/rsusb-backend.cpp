// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "rsusb-backend.h"

#include "../uvc/uvc-device.h"
#include "../types.h"
#include "../hid/hid-types.h"
#include "../hid/hid-device.h"
#include "../usb/usb-enumerator.h"

#include "../tm2/tm-boot.h"

#include <chrono>
#include <cctype> // std::tolower


namespace librealsense
{
    namespace platform
    {
        rs_backend::rs_backend()
        {

        }

        rs_backend::~rs_backend()
        {

        }

        std::shared_ptr<uvc_device> rs_backend::create_uvc_device(uvc_device_info info) const
        {
            LOG_DEBUG("Creating UVC Device from path: " << info.device_path.c_str());
            auto dev = create_rsuvc_device(info);
            if (!dev)
                return nullptr;
            return std::make_shared<retry_controls_work_around>(dev);
        }

        std::vector<uvc_device_info> rs_backend::query_uvc_devices() const {
            return query_uvc_devices_info();
        }

        std::shared_ptr<command_transfer> rs_backend::create_usb_device(usb_device_info info) const
         {
            auto dev = usb_enumerator::create_usb_device(info);
            if(dev != nullptr)
                return std::make_shared<command_transfer_usb>(dev);
            return nullptr;
        }

        std::vector<usb_device_info> rs_backend::query_usb_devices() const
        {
            auto device_infos = usb_enumerator::query_devices_info();
            // Give the device a chance to restart, if we don't catch
            // it, the watcher will find it later.
            if(tm_boot(device_infos)) {
                std::this_thread::sleep_for(std::chrono::milliseconds(2000));
                device_infos = usb_enumerator::query_devices_info();
            }
            return device_infos;
        }

        std::shared_ptr<hid_device> rs_backend::create_hid_device(hid_device_info info) const
        {
            return create_rshid_device(info);
        }

        std::vector<hid_device_info> rs_backend::query_hid_devices() const
        {
            return query_hid_devices_info();
        }

        std::shared_ptr<time_service> rs_backend::create_time_service() const
        {
            return std::make_shared<os_time_service>();
        }
    }
}
