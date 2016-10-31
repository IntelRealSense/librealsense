// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_RS450T_H
#define LIBREALSENSE_RS450T_H

#include "rs4xx.h"

#define DS5_AWGCT_PRODUCT_ID 0x0ad5

namespace rsimpl
{
    // This class represent DS5 camera SKUs which generate depth data, RGB frames and includes a tracking module.
    class rs450t_camera final : public rs4xx_camera
    {

    public:
        rs450t_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~rs450t_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
    };

    std::shared_ptr<rs_device> make_ds5t_device(std::shared_ptr<uvc::device> device);
}

#endif
