// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <chrono>
#include "ds5-color.h"
#include "ds5-thermal-handler.h"


namespace librealsense
{
    ds5_thermal_handler::ds5_thermal_handler( synthetic_sensor& activation_sensor) :
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            }),
        _poll_intervals_ms(1000),
        _streaming_on(false),
        _feature_on(false)
     {
        _dpt_sensor = std::dynamic_pointer_cast<synthetic_sensor>(activation_sensor.shared_from_this());

        auto& dev = activation_sensor.get_device();
        for (size_t i = 0; i < dev.get_sensors_count(); ++i)
        {
            if (auto s = dynamic_cast<ds5_recalibrable_color_sensor*>(&(dev.get_sensor(i))))
            {
                _recalib_sensor = std::dynamic_pointer_cast<ds5_recalibrable_color_sensor>(s->shared_from_this());
            }
        }

        // TODO Evgeni
        //if (_fw_version >= firmware_version("5.12.7.100"))
        {
            activation_sensor.register_option(RS2_OPTION_THERMAL_COMPENSATION, std::make_shared<thermal_compensation>(this));
        }
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
        // Enforce calibration reread on deactivation
        if (auto sp = _recalib_sensor.lock())
            sp->reset_calibration();
    }


    void ds5_thermal_handler::trigger_device_calibration(rs2_calibration_type type)
    {
        bool change_required = false;
        bool activate = (RS2_CALIBRATION_THERMAL == type);
        if (_streaming_on != activate)
        {
            if (auto sp = _dpt_sensor.lock())
            {
                // Thermal Activation occurs on depth sensor streaming changes
                change_required = (sp->is_opened()) && (!sp->is_streaming());
            }
            if (change_required)
            {
                _streaming_on = activate;
                update_mode(true);
            }
        }
    }

    void ds5_thermal_handler::set_feature(bool state)
    {
        bool change_required = false;
        if (state != _feature_on)
        {
            _feature_on = state;
            update_mode();
        }
    }

    void ds5_thermal_handler::update_mode(bool on_streaming)
    {
        if (_streaming_on && _feature_on)
            start();
        if ((!_streaming_on && _feature_on && on_streaming) ||
            (_streaming_on && !_feature_on && !on_streaming))
            stop();

    }

    void ds5_thermal_handler::polling(dispatcher::cancellable_timer cancellable_timer)
    {
        if (cancellable_timer.try_sleep(_poll_intervals_ms))
        {
            try
            {
                auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
                if (auto sp = _dpt_sensor.lock())
                {
                    auto val = static_cast<int16_t>(sp->get_option(RS2_OPTION_ASIC_TEMPERATURE).query());

                    if (_temp_records.empty() || fabs(_temp_base - val) >= 2.f)
                    {
                        if (auto recalib_p = _recalib_sensor.lock())
                            recalib_p->reset_calibration();

                        auto interval_sec = (_temp_records.size()) ? (ts - _temp_records.back().timestamp_ms) / 1000 : 0;
                        LOG_INFO("Thermal compensation was triggered on change from " << _temp_base << " to " << val
                                  << " Deg(c) after " << interval_sec << " seconds");
                        _temp_base = val;
                        _temp_records.push_back({ ts, val });
                        // Keep record of the last 10 threshold events
                        if (_temp_records.size() > 10)
                            _temp_records.pop_front();
                    }
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
