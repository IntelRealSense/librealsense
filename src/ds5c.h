// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5C_H
#define LIBREALSENSE_DS5C_H

#include "rs4xx.h"

#define DS5_ASRC_PRODUCT_ID 0x0ad3
#define DS5_AWGC_PRODUCT_ID 0x0ad4

namespace rsimpl
{
    // This class represent DS5 variants that generate depth data and RGB images. There are 2 variants:
    // (1) DS5 ASRC: Uses the OV2241 sensor that has a rolling shutter
    // (2) DS5 AWGC: Uses the OV9282 sensor that has a global shutter. This variant also has wide FOV
    class ds5c_camera final : public rs4xx_camera
    {

    public:
        ds5c_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, bool has_global_shutter);
        ~ds5c_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

    private:
        bool has_global_shutter;

    };

    std::shared_ptr<rs_device> make_ds5c_rolling_device(std::shared_ptr<uvc::device> device);
    std::shared_ptr<rs_device> make_ds5c_global_wide_device(std::shared_ptr<uvc::device> device);
}

#endif
