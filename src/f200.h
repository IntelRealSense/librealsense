#pragma once
#ifndef LIBREALSENSE_F200_H
#define LIBREALSENSE_F200_H

#include "device.h"
#include "f200-private.h" // TODO: Refactor so we don't need this here

#include <atomic>
#include <thread>
#include <condition_variable>

namespace rsimpl
{
    namespace f200 { class IVCAMHardwareIO; }

    class f200_camera : public rs_device
    {
        std::timed_mutex usbMutex;

        f200::CameraCalibrationParameters base_calibration;
        f200::IVCAMTemperatureData base_temperature_data;
        f200::IVCAMThermalLoopParams thermal_loop_params;

        float last_temperature_delta;

        std::thread temperatureThread;
        std::atomic<bool> runTemperatureThread;
        std::mutex temperatureMutex;
        std::condition_variable temperatureCv;

        void temperature_control_loop();
    public:      
        f200_camera(uvc::device_ref device, const static_device_info & info, const f200::CameraCalibrationParameters & calib, const f200::IVCAMTemperatureData & temp, const f200::IVCAMThermalLoopParams & params);
        ~f200_camera();

        void on_before_start(const std::vector<subdevice_mode> & selected_modes) override final;
        void set_option(rs_option option, int value) override final;
        int get_option(rs_option option) const override final;
        int convert_timestamp(const rsimpl::stream_request (& requests)[RS_STREAM_COUNT], int64_t timestamp) const override final { return static_cast<int>(timestamp / 100000); }
    };

    std::shared_ptr<rs_device> make_f200_device(uvc::device_ref device);
    std::shared_ptr<rs_device> make_sr300_device(uvc::device_ref device);
}

#endif
