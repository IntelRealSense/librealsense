// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "../usb/usb-device.h"

#include <vector>
#include <memory>

namespace librealsense
{
    namespace platform
    {
        class usb_enumerator
        {
        public:
            static std::vector<std::shared_ptr<usb_device>> query_devices();
            static bool is_device_connected(const std::shared_ptr<usb_device> device);
        };
    }
}