// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include <atomic>
#include <thread>
#include <condition_variable>

#include "ivcam-device.h"


#define F200_PRODUCT_ID  0x0a66

namespace rsimpl
{
    class f200_camera final : public iv_camera
    {
        f200::cam_temperature_data base_temperature_data;
        f200::thermal_loop_params thermal_loop_params;

        float last_temperature_delta;

        std::thread temperatureThread;
        std::atomic<bool> runTemperatureThread;
        std::mutex temperatureMutex;
        std::condition_variable temperatureCv;

        void temperature_control_loop();

    public:
        f200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const ivcam::camera_calib_params & calib, const f200::cam_temperature_data & temp, const f200::thermal_loop_params & params);
        ~f200_camera();

        void set_options(const rs_option options[], size_t count, const double values[]) override;
        void get_options(const rs_option options[], size_t count, double values[]) override;

        virtual void start_fw_logger(char fw_log_op_code, int grab_rate_in_ms, std::timed_mutex& mutex) override;
        virtual void stop_fw_logger() override;
    };

    std::shared_ptr<rs_device> make_f200_device(std::shared_ptr<uvc::device> device);
}

#endif
