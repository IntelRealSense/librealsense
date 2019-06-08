// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "l500-private.h"

namespace librealsense
{
    class l500_info : public device_info
    {
    public:
        std::shared_ptr<device_interface> create(std::shared_ptr<context> ctx,
            bool register_device_notifications) const override;

        l500_info(std::shared_ptr<context> ctx,
            std::vector<platform::uvc_device_info> depth,
            platform::usb_device_info hwm,
            std::vector<platform::hid_device_info> hid)
            : device_info(ctx),
            _depth(std::move(depth)),
            _hwm(std::move(hwm)),
            _hid(std::move(hid))
        {}

        static std::vector<std::shared_ptr<device_info>> pick_l500_devices(
            std::shared_ptr<context> ctx,
            platform::backend_device_group& group);

        platform::backend_device_group get_device_data() const override
        {
            std::vector<platform::usb_device_info> usb_devices;
            if (_hwm.id != "")
                usb_devices = { _hwm };
            return platform::backend_device_group({ _depth }, usb_devices, { _hid });
        }

    private:
        std::vector<platform::uvc_device_info> _depth;
        platform::usb_device_info _hwm;
        std::vector<platform::hid_device_info> _hid;
    };
}
