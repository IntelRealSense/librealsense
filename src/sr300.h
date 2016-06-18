// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_SR300_H
#define LIBREALSENSE_SR300_H

#include <atomic>
#include <thread>


#include "iv-camera.h"
#include "sr300-private.h" // todo - refactor so we don't need this here

#define SR300_PRODUCT_ID 0x0aa5

namespace rsimpl
{

    class sr300_camera final : public iv_camera
    {

        sr300::wakeup_dev_params arr_wakeup_dev_param;

    public:
        sr300_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const iv::camera_calib_params & calib);
        ~sr300_camera();

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

    };

    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device);
}

#endif
