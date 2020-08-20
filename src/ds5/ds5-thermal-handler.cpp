// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <chrono>
#include "ds5-color.h"
#include "ds5-thermal-handler.h"


namespace librealsense
{
    ds5_thermal_handler::ds5_thermal_handler( synthetic_sensor& activation_sensor/*, synthetic_sensor& recalibration_sensor*/) :
        _activation_sensor(activation_sensor),
        //_affected_sensor(nullptr),
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            }),
        _poll_intervals_ms(1000)
     {
        _affected_sensor = [this]() {
            auto& dev = _activation_sensor.get_device();
            for (size_t i = 0; i < dev.get_sensors_count(); ++i)
            {
                if (auto s = dynamic_cast<const ds5_recalibrable_color_sensor*>(&(dev.get_sensor(i))))
                {
                    return const_cast<ds5_recalibrable_color_sensor*>(s);
                }
            }
            return (ds5_recalibrable_color_sensor*)nullptr;
        };

        //if (_fw_version >= firmware_version("5.12.7.100"))
        {
         /*   depth_sensor.register_option(RS2_OPTION_THERMAL_COMPENSATION,
                std::make_shared<uvc_xu_option<uint8_t>>(raw_depth_sensor, depth_xu, DS5_THERMAL_COMPENSATION,
                    "Toggle Depth Sensor Thermal Compensation"));*/
        }

        _activation_sensor.register_option(RS2_OPTION_THERMAL_COMPENSATION, std::make_shared<thermal_compensation>(this));
    }

    ds5_thermal_handler::~ds5_thermal_handler()
    {
        stop();
    }

    void ds5_thermal_handler::start()
    {
        auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::cout << __FUNCTION__ << " " << ts << std::endl;
        _active_object.start();
    }
    void ds5_thermal_handler::stop()
    {
        auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::cout << __FUNCTION__ << " " << ts << std::endl;
        _active_object.stop();
        _temp_records.clear();
    }

    void ds5_thermal_handler::polling(dispatcher::cancellable_timer cancellable_timer)
    {
        auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
        if (cancellable_timer.try_sleep(_poll_intervals_ms))
        {
            try
            {
                auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
                auto val = _activation_sensor.get_option(RS2_OPTION_ASIC_TEMPERATURE).query();

                std::cout << "Temperature is " << val << std::endl;
                if (_temp_records.empty() || fabs(_temp_base - val) >= 2.f)
                {
                    _temp_base = val;
                    (*_affected_sensor)->reset_calibration();
                    _temp_records.push_back({ ts, val });
                    // Keep record of the last 10 threshold events
                    if (_temp_records.size() > 10)
                        _temp_records.pop_front();
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("Error during thermal compensation handling: " << ex.what());
            }
            catch (...)
            {
                LOG_ERROR("Unknown error during thermal compensation handling!");
            }
        }
        else
        {
            LOG_DEBUG("Notification polling loop is being shut-down");
        }
    }



}
