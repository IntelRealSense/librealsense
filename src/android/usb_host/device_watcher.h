/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include "usbhost.h"
#include "usb_device.h"
#include "../../types.h"
#include <memory>

namespace librealsense
{
    namespace usb_host
    {
        class device_watcher : public platform::device_watcher
        {
        public:
            virtual void start(platform::device_changed_callback callback) override;
            virtual void stop() override;
            static std::vector<std::shared_ptr<device>> get_device_list();
            static std::vector<platform::uvc_device_info> query_uvc_devices();
        };
    }
}
