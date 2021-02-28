// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <chrono>
#include "ds5-color.h"
#include "ds5-private.h"
#include "ds5-thermal-monitor.h"

namespace librealsense
{
    ds5_thermal_monitor::ds5_thermal_monitor(synthetic_sensor& activation_sensor,
                                            std::shared_ptr<option> temp_option) :
        _monitor([this](dispatcher::cancellable_timer cancellable_timer)
            {
                polling(cancellable_timer);
            }),
        _poll_intervals_ms(2000), // Temperature check routine to be invoked every 2 sec
        _thermal_threshold_deg(2.f),
        _temp_base(0.f),
        _is_running(false),
        _temperature_sensor(temp_option)
     {
        _dpt_sensor = std::dynamic_pointer_cast<synthetic_sensor>(activation_sensor.shared_from_this());
        /*_dpt_sensor = std::dynamic_pointer_cast<synthetic_sensor>(activation_sensor.shared_from_this());

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

        activation_sensor.register_option(RS2_OPTION_THERMAL_COMPENSATION, std::make_shared<thermal_compensation>(this));*/
    }

    ds5_thermal_monitor::~ds5_thermal_monitor()
    {
        stop();
    }

    void ds5_thermal_monitor::start()
    {
        if (!_is_running)
        {
            _monitor.start();
            _is_running = true;
        }
    }

    void ds5_thermal_monitor::stop()
    {
        if (_is_running)
        {
            _monitor.stop();
            _temp_base = 0.f;
            _is_running = false;
        }
        // Enforce calibration re-read on deactivation
        /*if (auto sp = _recalib_sensor.lock())
            sp->reset_calibration();*/

        //notify_of_calibration_change(RS2_CALIBRATION_SUCCESSFUL);
    }

    void ds5_thermal_monitor::update(bool on)
    {
        if (on != is_running())
        {
            if (auto snr = _dpt_sensor.lock())
            {
                if (!on)
                {
                    stop();
                    notify(0);
                }
                else
                    if (snr->is_opened())
                        start();
            }
        }
    }

    void ds5_thermal_monitor::polling(dispatcher::cancellable_timer cancellable_timer)
    {
        if (cancellable_timer.try_sleep(_poll_intervals_ms))
        {
            try
            {
                auto ts = (uint64_t)std::chrono::high_resolution_clock::now().time_since_epoch().count();
                if (auto temp = _temperature_sensor.lock())
                {
                    auto cur_temp = temp->query();

                    if (fabs(_temp_base - cur_temp) >= _thermal_threshold_deg)
                    {
                        LOG_DEBUG("Thermal calibration adjustment is triggered on change from "
                            << std::dec << std::setprecision(1) << _temp_base << " to " << cur_temp << " deg (C)");

                        notify(cur_temp);
                        _temp_base = cur_temp;
                    }
                }
                else
                {
                    LOG_ERROR("Thermal Compensation: temperature sensor option is not present");
                }
            }
            catch (const std::exception& ex)
            {
                LOG_ERROR("Error during thermal compensation handling: " << ex.what());
            }
            catch (...)
            {
                LOG_ERROR("Unresolved error during Thermal Compensation handling");
            }
        }
        else
        {
            LOG_DEBUG("Thermal Compensation is being shut-down");
        }
    }

    void ds5_thermal_monitor::notify(float temperature) const
    {
        for (auto&& cb : _thermal_changes_callbacks)
            cb(temperature);
    }
}
