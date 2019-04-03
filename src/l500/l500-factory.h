// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "l500-private.h"
#include "l500-depth.h"

namespace librealsense
{
    class l500_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
            bool register_device_notifications) const override;

        l500_info(std::shared_ptr<context> ctx,
            std::vector<platform::uvc_device_info> depth,
            platform::usb_device_info hwm)
            : device_info(ctx),
            _depth(std::move(depth)),
            _hwm(std::move(hwm))
        {}

        static std::vector<std::shared_ptr<device_info>> pick_l500_devices(
            std::shared_ptr<context> ctx,
            std::vector<platform::uvc_device_info>& platform,
            std::vector<platform::usb_device_info>& usb);

        platform::backend_device_group get_device_data() const override
        {
            return platform::backend_device_group({ _depth }, { _hwm });
        }

    private:
        std::vector<platform::uvc_device_info> _depth;

        platform::usb_device_info _hwm;
    };
}
