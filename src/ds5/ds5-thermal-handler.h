// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.
#pragma once

#include <list>
#include "ds5-device.h"
#include "device-calibration.h"

namespace librealsense
{
    class ds5_recalibrable_color_sensor;
    class ds5_thermal_handler : public virtual device_calibration
    {
    public:
        ds5_thermal_handler(synthetic_sensor& activation_sensor/*, synthetic_sensor& recalibration_sensor*/);

        virtual ~ds5_thermal_handler();

        void start();
        void stop();

        void register_calibration_change_callback(calibration_change_callback_ptr callback) override
        {
            _calibration_change_callbacks.push_back(callback);
        }

        void trigger_device_calibration(rs2_calibration_type) override
        {
            //TODO - identical to set option = true
            //throw librealsense::not_implemented_exception("depth sensor does not support intrinsics override");
        }

        //void notify_of_calibration_change(rs2_calibration_status status);
    private:

        void polling(dispatcher::cancellable_timer cancellable_timer);

        synthetic_sensor&                           _activation_sensor;
        lazy<ds5_recalibrable_color_sensor*>        _affected_sensor;

        std::vector< calibration_change_callback_ptr > _calibration_change_callbacks;   // End-user updates to track calibration changes

        active_object<> _active_object;
        unsigned int _poll_intervals_ms;
        struct temperature_record
        {
            uint64_t    timestamp_ms;
            float       temp_celcius;
        };
        std::list<temperature_record> _temp_records;
        float _temp_base;
    };

}
