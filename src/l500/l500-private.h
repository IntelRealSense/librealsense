// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#pragma once

#include "backend.h"
#include "types.h"

const double TIMESTAMP_USEC_TO_MSEC = 0.001;

namespace librealsense
{
    namespace ivcam2
    {
        const uint16_t L500_PID = 0x0b0d;

        enum fw_cmd : uint8_t
        {
            HWReset = 0x20,
            GVD = 0x10,
            GLD = 0x0f
        };

        enum gvd_fields
        {
            fw_version_offset = 12,
            module_serial_offset = 64
        };

        bool try_fetch_usb_device(std::vector<platform::usb_device_info>& devices,
                                         const platform::uvc_device_info& info, platform::usb_device_info& result);
    } // librealsense::ivcam2
} // namespace librealsense
