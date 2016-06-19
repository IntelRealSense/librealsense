// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_DS_CAMERA_H
#define LIBREALSENSE_DS_CAMERA_H

#include "device.h"
#include "ds-common.h"

#define R200_PRODUCT_ID  0x0a80
#define LR200_PRODUCT_ID 0x0abf
#define ZR300_PRODUCT_ID 0x0acb
#define FISHEYE_PRODUCT_ID 0x0ad0

namespace rsimpl
{
    /*
    ds_camera class is interface that provides partial implementation for ds cameras line functionalities and properties
    */
    class ds_camera : public rs_device_base
    {
    protected:
        // This single function interface enforces implementation by subclasses
        //virtual std::shared_ptr<rs_device> make_device(std::shared_ptr<uvc::device> device, static_device_info& info, ds::ds_calibration& c) abstract;
        bool is_disparity_mode_enabled() const;
        void on_update_depth_units(uint32_t units);
        void on_update_disparity_multiplier(double multiplier);
        uint32_t get_lr_framerate() const;

    public:
        ds_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~ds_camera();

        bool supports_option(rs_option option) const override;
        void get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        std::shared_ptr<frame_timestamp_reader> create_frame_timestamp_reader() const override;

        static void set_common_ds_config(std::shared_ptr<uvc::device> device, static_device_info& info, const ds::ds_calibration& c);
    };
}

#endif // DS_CAMERA_H
