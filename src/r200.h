// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_R200_H
#define LIBREALSENSE_R200_H

#include "ds-device.h"

namespace rsimpl
{
    class r200_camera final : public ds::ds_device
    {

    public:
        r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~r200_camera() {};

        virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
        virtual void stop_fw_logger() override;
    };

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device);
    std::shared_ptr<rs_device> make_lr200_device(std::shared_ptr<uvc::device> device);
}

#endif
