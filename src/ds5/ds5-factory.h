// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once

#include "ds5-rolling-shutter.h"
#include "ds5-active.h"
#include "ds5-motion.h"
#include "ds5-color.h"

#include "algo.h"
#include "error-handling.h"

namespace rsimpl2
{
    class ds5_info : public device_info
    {
    public:
        std::shared_ptr<device> create(const uvc::backend& backend) const override;

        uint8_t get_subdevice_count() const override
        {
            auto depth_pid = _depth.front().pid;
            switch(depth_pid)
            {
            case ds::RS400_PID:
            case ds::RS410_PID:
            case ds::RS430_PID:
            case ds::RS420_PID:
                return 1;
            case ds::RS415_PID:
            case ds::RS435_RGB_PID:
                return 2;
            case ds::RS430_MM_PID:
            case ds::RS420_MM_PID:
                return 3;
            default:
                throw not_implemented_exception(to_string() <<
                    "get_subdevice_count is not implemented for DS5 device of type " <<
                    depth_pid);
            }
        }

        ds5_info(std::shared_ptr<uvc::backend> backend,
                 std::vector<uvc::uvc_device_info> depth,
                 std::vector<uvc::usb_device_info> hwm,
                 std::vector<uvc::hid_device_info> hid)
            : device_info(std::move(backend)), _hwm(std::move(hwm)),
              _depth(std::move(depth)), _hid(std::move(hid)) {}

        static std::vector<std::shared_ptr<device_info>> pick_ds5_devices(
                std::shared_ptr<uvc::backend> backend,
                std::vector<uvc::uvc_device_info>& uvc,
                std::vector<uvc::usb_device_info>& usb,
                std::vector<uvc::hid_device_info>& hid);

        uvc::devices_data get_device_data() const override
        {
            return uvc::devices_data(_depth, _hwm, _hid);
        }
    private:
        std::vector<uvc::uvc_device_info> _depth;
        std::vector<uvc::usb_device_info> _hwm;
        std::vector<uvc::hid_device_info> _hid;
    };
}
