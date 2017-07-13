// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-device.h"

namespace librealsense
{
    class ds5_active : public virtual ds5_device
    {
    public:
        ds5_active(const uvc::backend& backend,
            const std::vector<uvc::uvc_device_info>& dev_info,
            const std::vector<uvc::usb_device_info>& hwm_device,
            const std::vector<uvc::hid_device_info>& hid_info);

        std::shared_ptr<matcher> create_matcher(rs2_stream stream) const;
       
    };
}
