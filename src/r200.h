/*
    INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
    terms of a license agreement or nondisclosure agreement with Intel Corporation
    and may not be copied or disclosed except in accordance with the terms of that
    agreement.
    Copyright(c) 2015 Intel Corporation. All Rights Reserved.
*/

#pragma once
#ifndef LIBREALSENSE_R200_H
#define LIBREALSENSE_R200_H

#include "device.h"

namespace rsimpl
{
    class r200_camera : public rs_device
    {
        bool is_disparity_mode_enabled() const;
        void on_update_depth_units(int units);
        void on_update_disparity_multiplier(float multiplier);
        int get_lr_framerate() const;
    public:
        r200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, std::vector<rs_intrinsics> intrinsics, std::vector<rs_intrinsics> rect_intrinsics);
        ~r200_camera();

        void on_before_start(const std::vector<subdevice_mode> & selected_modes) override final;
        void get_xu_range(rs_option option, int * min, int * max) const override final;
        void set_xu_option(rs_option option, int value) override final;
        int get_xu_option(rs_option option) const override final;
        int convert_timestamp(int64_t timestamp) const override final;
    };

    std::shared_ptr<rs_device> make_r200_device(std::shared_ptr<uvc::device> device);
}

#endif
