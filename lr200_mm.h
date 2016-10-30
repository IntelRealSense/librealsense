// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_LR200_MM_H
#define LIBREALSENSE_LR200_MM_H

#include "ds-device.h"
#include "motion-common.h"



namespace rsimpl
{
    

    class lr200_mm_camera final : public ds::ds_device
    {

    public:
        lr200_mm_camera(std::shared_ptr<uvc::device> device, const static_device_info & info);
        ~lr200_mm_camera() {};

        void get_option_range(rs_option option, double & min, double & max, double & step, double & def) override;
        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;
        virtual void start_motion_tracking() override;
        virtual void stop_motion_tracking() override;
        virtual void start_video_streaming(bool is_mipi = false);
        virtual void stop_video_streaming();
        void start(rs_source source) override;
        void stop(rs_source source) override;
        void initialize_motion() override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;

        rs_motion_intrinsics get_motion_intrinsics() const override;
        rs_extrinsics get_motion_extrinsics_from(rs_stream from) const override;
        
        virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
        virtual void stop_fw_logger() override;
   
        motion_module_calibration fe_intrinsic;
        bool motion_initialized = false;
        motion::MotionProfile profile;
        bool is_motion_initialized = false;

    };


    std::shared_ptr<rs_device> make_lr200_mm_device(std::shared_ptr<uvc::device> device);
}

#endif
