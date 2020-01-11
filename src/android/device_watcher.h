/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once

#include "../types.h"
#include "../backend.h"
#include "../usb/usb-device.h"

#include <memory>

namespace librealsense
{
    namespace platform
    {
        class device_watcher_usbhost : public device_watcher
        {
        public:
            virtual void start(device_changed_callback callback) override;
            virtual void stop() override;

            void notify();
            static std::shared_ptr<device_watcher_usbhost> instance();

        private:
            std::mutex _mutex;
            device_changed_callback _callback = nullptr;
            backend_device_group _prev_group;
            std::vector<platform::uvc_device_info> update_uvc_devices();
        };
    }
}
