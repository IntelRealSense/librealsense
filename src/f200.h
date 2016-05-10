// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include "device.h"
#include "f200-private.h" // todo - refactor so we don't need this here

#include <atomic>
#include <thread>
#include <condition_variable>

const std::vector <std::pair<rs_option, char>> eu_F200_depth_controls = {{rs_option::RS_OPTION_F200_LASER_POWER,          0x01},
                                                                         {rs_option::RS_OPTION_F200_ACCURACY,             0x02},
                                                                         {rs_option::RS_OPTION_F200_MOTION_RANGE,         0x03},
                                                                         {rs_option::RS_OPTION_F200_FILTER_OPTION,        0x05},
                                                                         {rs_option::RS_OPTION_F200_CONFIDENCE_THRESHOLD, 0x06},
                                                                         {rs_option::RS_OPTION_F200_DYNAMIC_FPS,          0x07}};

const std::vector <std::pair<rs_option, char>> eu_SR300_depth_controls = {{rs_option::RS_OPTION_F200_LASER_POWER,          0x01},
                                                                          {rs_option::RS_OPTION_F200_ACCURACY,             0x02},
                                                                          {rs_option::RS_OPTION_F200_MOTION_RANGE,         0x03},
                                                                          {rs_option::RS_OPTION_F200_FILTER_OPTION,        0x05},
                                                                          {rs_option::RS_OPTION_F200_CONFIDENCE_THRESHOLD, 0x06}};

namespace rsimpl
{
    namespace f200 { class IVCAMHardwareIO; }

    class f200_camera final : public rs_device
    {
        std::timed_mutex usbMutex;

        f200::CameraCalibrationParameters base_calibration;
        f200::IVCAMTemperatureData base_temperature_data;
        f200::IVCAMThermalLoopParams thermal_loop_params;
        f200::IVCAMAutoRangeRequest arr;
        sr300::wakeup_dev_params arr_wakeup_dev_param;

        float last_temperature_delta;

        std::thread temperatureThread;
        std::atomic<bool> runTemperatureThread;
        std::mutex temperatureMutex;
        std::condition_variable temperatureCv;

        void temperature_control_loop();
    public:      
        f200_camera(std::shared_ptr<uvc::device> device, const static_device_info & info, const f200::CameraCalibrationParameters & calib, const f200::IVCAMTemperatureData & temp, const f200::IVCAMThermalLoopParams & params);
        ~f200_camera();

        void on_before_start(const std::vector<subdevice_mode_selection> & selected_modes) override;
        rs_stream select_key_stream(const std::vector<rsimpl::subdevice_mode_selection> & selected_modes) override;
        
        void set_options(const rs_option options[], int count, const double values[]) override;
        void get_options(const rs_option options[], int count, double values[]) override;

        std::shared_ptr<frame_timestamp_reader> create_frame_timestamp_reader() const override;
    };

    std::shared_ptr<rs_device> make_f200_device(std::shared_ptr<uvc::device> device);
    std::shared_ptr<rs_device> make_sr300_device(std::shared_ptr<uvc::device> device);
}

#endif
