// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "r200.h"

using namespace rsimpl;
using namespace rsimpl::ds;

namespace rsimpl
{
    r200_camera::r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) 
    : ds_device(device, info)
    {

    }

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense R200");

        static_device_info info;
        info.name = { "Intel RealSense R200" };
        auto c = ds::read_camera_info(*device);
        // R200 provides Full HD raw 10 format, its descriptors is defined as follows
        info.subdevice_modes.push_back({ 2, {2400, 1081},  pf_rw10, 30, c.intrinsicsThird[0], {c.modesThird[0][0]}, {0}});

        ds_device::set_common_ds_config(device, info, c);
        return std::make_shared<r200_camera>(device, info);
    }

    std::shared_ptr<rs_device> make_lr200_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense LR200");

        static_device_info info;
        info.name = { "Intel RealSense LR200" };
        auto c = ds::read_camera_info(*device);
        // LR200 provides Full HD raw 16 format as well for the color stream
        info.subdevice_modes.push_back({ 2,{ 1920, 1080 }, pf_rw16, 30, c.intrinsicsThird[0],{ c.modesThird[0][0] },{ 0 } });

        ds_device::set_common_ds_config(device, info, c);
        return std::make_shared<r200_camera>(device, info);
    }
}
