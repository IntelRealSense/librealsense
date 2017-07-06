// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#include <mutex>
#include <chrono>
#include <vector>
#include <iterator>
#include <cstddef>

#include "device.h"
#include "context.h"
#include "image.h"
#include "metadata-parser.h"

#include "ds5-rolling-shutter.h"
#include "ds5-private.h"
#include "ds5-options.h"
#include "ds5-timestamp.h"

namespace librealsense
{
    ds5_rolling_shutter::ds5_rolling_shutter(const uvc::backend& backend,
                           const std::vector<uvc::uvc_device_info>& dev_info,
                           const std::vector<uvc::usb_device_info>& hwm_device,
                           const std::vector<uvc::hid_device_info>& hid_info)
        : ds5_device(backend, dev_info, hwm_device, hid_info)
    {
        using namespace ds;

        if (_fw_version >= firmware_version("5.5.8.0"))
        {
            get_depth_sensor().register_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE,
                std::make_shared<uvc_xu_option<uint8_t>>(get_depth_sensor(),
                                                         depth_xu,
                                                         DS5_ENABLE_AUTO_WHITE_BALANCE,
                                                         "Enable Auto White Balance"));
        }
    }
}
