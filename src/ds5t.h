// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5T_H
#define LIBREALSENSE_DS5T_H

#include "ds5.h"

#define DS5_AWGCT_PRODUCT_ID 0x0ad5

namespace rsimpl
{
    // This class represent DS5 camera SKUs which generate depth data, RGB frames and includes a tracking module.
    class ds5t_camera final : public ds5_camera
    {

    public:
        ds5t_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~ds5t_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

    };

    std::shared_ptr<rs_device> make_ds5t_device(std::shared_ptr<uvc::device> device);
}

#endif
