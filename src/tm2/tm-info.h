// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.
#pragma once

#include <memory>
#include <vector>

#include "../context.h"
#include "tm-device.h"

namespace librealsense
{
    class tm2_info : public device_info
    {
    public:
        tm2_info(std::shared_ptr<context> ctx, platform::usb_device_info hwm);
        ~tm2_info();
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx, bool register_device_notifications) const override;
        platform::backend_device_group get_device_data() const override;
        
        static std::vector<std::shared_ptr<device_info>> pick_tm2_devices(
            std::shared_ptr<context> ctx,
            std::vector<platform::usb_device_info>& usb);
    private:
        platform::usb_device_info _hwm;
    };
}
