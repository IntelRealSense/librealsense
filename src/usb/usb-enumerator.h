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
            static rs_usb_device create_usb_device(const usb_device_info& info);
            static std::vector<usb_device_info> query_devices_info();
        };
    }
}