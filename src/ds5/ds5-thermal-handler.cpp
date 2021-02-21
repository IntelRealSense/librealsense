// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <chrono>
#include "ds5-color.h"
#include "ds5-private.h"
#include "ds5-thermal-handler.h"

namespace librealsense
{
    ds5_thermal_handler::ds5_thermal_handler( synthetic_sensor& activation_sensor) :
        _active_object([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            }),
        _poll_intervals_ms(2000), // Temperature check routine to be invoked every 2 sec
        _thermal_threshold_deg(2.f),
        _temp_base(0.f),
        _streaming_on(false)
     {
        _dpt_sensor = std::dynamic_pointer_cast<synthetic_sensor>(activation_sensor.shared_from_this());

        auto& dev = activation_sensor.get_device();

        for (size_t i = 0; i < dev.get_sensors_count(); ++i)
        {
            if (auto s = dynamic_cast<ds5_recalibrable_color_sensor*>(&(dev.get_sensor(i))))
            {
                _recalib_sensor = std::dynamic_pointer_cast<ds5_recalibrable_color_sensor>(s->shared_from_this());
                break;
            }
        }

        _tl_activation = std::make_shared<uvc_xu_option<uint8_t>>(dynamic_cast<uvc_sensor&>(*activation_sensor.get_raw_sensor()),
            ds::depth_xu, ds::DS5_THERMAL_COMPENSATION, "Toggle Thermal Compensation Mechanism");

        try {
            _control_on = static_cast<bool>(_tl_activation->query());
        }
        catch (...)
        {
            _control_on = true;
            LOG_WARNING("Initial TL Control state could not be verified, assume on ");
        }

        activation_sensor.register_option(RS2_OPTION_THERMAL_COMPENSATION, std::make_shared<thermal_compensation>(this));
    }

    ds5_thermal_handler::~ds5_thermal_handler()
    {
        if (_streaming_on && _control_on)
            stop();
    }

    void ds5_thermal_handler::start()
    {
        _active_object.start();
    }

    void ds5_thermal_handler::stop()
    {
        _active_object.stop();
        _temp_base = 0.f;

        // Enforce calibration re-read on deactivation
        if (auto sp = _recalib_sensor.lock())
            sp->reset_calibration();

        notify_of_calibration_change(RS2_CALIBRATION_SUCCESSFUL);
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
                LOG_DEBUG("Thermal compensation polling was changed from " << (int)_streaming_on << " to " << (int)activate);
                _streaming_on = activate;
                update_mode(true);
            }
        }
    }

    void ds5_thermal_handler::set_feature(bool state)
    {
        bool change_required = false;
        LOG_DEBUG(__FUNCTION__ << " state = " << state << " _control_on = " << _control_on);
        if (state != _control_on)
        {
            _tl_activation->set(state);
            _control_on = state;
            update_mode();
        }
    }

    float ds5_thermal_handler::query()
    {
        auto ctrl_state = static_cast<bool>(_tl_activation->query());
        LOG_DEBUG(__FUNCTION__ << " _control_on = " << _control_on << " ctrl_state = " << ctrl_state);
        if (_control_on != ctrl_state)
        {
            _control_on = ctrl_state;
            update_mode();
        }
        return ctrl_state;
    }


    void ds5_thermal_handler::update_mode(bool on_streaming)
    {
        if (_streaming_on && _control_on)
        {
            start();
            return;
        }
        // Deactivate when toggling off streaming or control
        if ((!_streaming_on && _control_on && on_streaming) ||
            (_streaming_on && !_control_on && !on_streaming))
        {
            stop();
            return;
        }

        LOG_ERROR("Thermal compensation update_mode was not changed: on_streaming =" << on_streaming
                    << ", _streaming_on=" << _streaming_on << ", _control_on=" << _control_on);
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
                    auto cur_temp = sp->get_option(RS2_OPTION_ASIC_TEMPERATURE).query();

                    if (fabs(_temp_base - cur_temp) >= _thermal_threshold_deg)
                    {
                        if (auto recalib_p = _recalib_sensor.lock())
                        {
                            LOG_INFO("Thermal calibration adjustment is triggered on change from "
                                << std::dec << _temp_base << " to " << cur_temp << " deg (C)");
                            recalib_p->reset_calibration();

                            notify_of_calibration_change(RS2_CALIBRATION_SUCCESSFUL);

                            _temp_base = cur_temp;
                        }
                    }
                }
                else
                {
                    LOG_ERROR("Thermal Compensation: Depth sensor is not present");
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("Error during thermal compensation handling: " << ex.what());
            }
            catch (...)
            {
                LOG_ERROR("Unresolved error during Thermal Compensation handling!");
            }
        }
        else
        {
            LOG_DEBUG("Thermal Compensationis being shut-down");
        }
    }

    void ds5_thermal_handler::notify_of_calibration_change(rs2_calibration_status status)
    {
        for (auto&& cb : _calibration_change_callbacks)
            cb->on_calibration_change(status);
    }
}
