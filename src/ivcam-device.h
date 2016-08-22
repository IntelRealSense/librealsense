// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_IV_CAMERA_H
#define LIBREALSENSE_IV_CAMERA_H

#include "ivcam-private.h"
#include "device.h"

namespace rsimpl
{
    struct cam_mode { int2 dims; std::vector<int> fps; };

    rs_intrinsics MakeDepthIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims);
    rs_intrinsics MakeColorIntrinsics(const ivcam::camera_calib_params & c, const int2 & dims);
    void update_supported_options(uvc::device& dev,
        const uvc::extension_unit depth_xu,
        const std::vector <std::pair<rs_option, char>> options,
        std::vector<supported_option>& supported_options);

    class iv_camera : public rs_device_base
    {
    protected:
        std::timed_mutex usbMutex;

        ivcam::camera_calib_params base_calibration;
        ivcam::cam_auto_range_request arr;

    public:
        iv_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib);
        ~iv_camera() {};

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        std::vector<std::shared_ptr<frame_timestamp_reader>> create_frame_timestamp_readers() const override;

        void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
        void stop_fw_logger() override;
    };
}

#endif  // IV_CAMERA_H
