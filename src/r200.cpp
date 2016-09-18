// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "image.h"
#include "r200.h"

using namespace rsimpl;
using namespace rsimpl::ds;

namespace rsimpl
{
    r200_camera::r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info) 
        : ds_device(device, info, calibration_validator())
    {
    }

    void r200_camera::start_fw_logger(char /*fw_log_op_code*/, int /*grab_rate_in_ms*/, std::timed_mutex& /*mutex*/)
    {
        throw std::logic_error("Not implemented");
    }

    void r200_camera::stop_fw_logger()
    {
        throw std::logic_error("Not implemented");
    }

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense R200");

        static_device_info info;
        info.name =  "Intel RealSense R200" ;
        auto cam_info = ds::read_camera_info(*device);

        ds_device::set_common_ds_config(device, info, cam_info);

        // R200 provides Full HD raw 10 format, its descriptors is defined as follows
        info.subdevice_modes.push_back({ 2, {2400, 1081},  pf_rw10, 30, cam_info.calibration.intrinsicsThird[0], { cam_info.calibration.modesThird[0][0]}, {0}});

        return std::make_shared<r200_camera>(device, info);
    }

    std::shared_ptr<rs_device> make_lr200_device(std::shared_ptr<uvc::device> device)
    {
        LOG_INFO("Connecting to Intel RealSense LR200");

        static_device_info info;
        info.name = "Intel RealSense LR200" ;
        auto cam_info = ds::read_camera_info(*device);

        ds_device::set_common_ds_config(device, info, cam_info);

        // LR200 provides Full HD raw 16 format as well for the color stream
        info.subdevice_modes.push_back({ 2,{ 1920, 1080 }, pf_rw16, 30, cam_info.calibration.intrinsicsThird[0],{ cam_info.calibration.modesThird[0][0] },{ 0 } });

        return std::make_shared<r200_camera>(device, info);
    }
}
