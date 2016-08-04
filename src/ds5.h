// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS5_H
#define LIBREALSENSE_DS5_H

#include "device.h"

namespace rsimpl
{
    // This is a base class for the various SKUs of the DS5 camera
    class ds5_camera : public rs_device_base
    {

    public:
        ds5_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~ds5_camera() {};

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        std::shared_ptr<frame_timestamp_reader> create_frame_timestamp_reader(int subdevice) const override;
    };
}

#endif
