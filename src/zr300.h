// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_ZR300_H
#define LIBREALSENSE_ZR300_H

#include "motion-module.h"
#include "ds-device.h"


#define R200_PRODUCT_ID     0x0a80
#define LR200_PRODUCT_ID    0x0abf
#define ZR300_PRODUCT_ID    0x0acb
#define FISHEYE_PRODUCT_ID  0x0ad0

namespace rsimpl
{
    class zr300_camera final : public ds::ds_device
    {

        motion_module::motion_module_control    motion_module_ctrl;
        motion_module::mm_config                motion_module_configuration;

    protected:
        void toggle_motion_module_power(bool bOn);
        void toggle_motion_module_events(bool bOn);

    public:
        zr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~zr300_camera();

        void get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        void start_motion_tracking() override;
        void stop_motion_tracking() override;

        void start(rs_source source) override;
        void stop(rs_source source) override;

        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
    };

    std::shared_ptr<rs_device> make_zr300_device(std::shared_ptr<uvc::device> device);
}

#endif
