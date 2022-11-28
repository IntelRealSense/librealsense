// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2016 Intel Corporation. All Rights Reserved.


#include "ds/ds5/ds5-thermal-monitor.h"
#include "ds5-options.h"

namespace librealsense
{
    auto_exposure_limit_option::auto_exposure_limit_option(hw_monitor& hwm, sensor_base* ep, option_range range, std::shared_ptr<limits_option> exposure_limit_enable)
        : option_base(range), _hwm(hwm), _sensor(ep), _exposure_limit_toggle(exposure_limit_enable)
    {
        _range = [range]()
        {
            return range;
        };
        if (auto toggle = _exposure_limit_toggle.lock())
            toggle->set_cached_limit(range.max);
    }

    void auto_exposure_limit_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception("set(enable_auto_exposure) failed! Invalid Auto-Exposure mode request " + std::to_string(value));

        if (auto toggle = _exposure_limit_toggle.lock())
        {
            toggle->set_cached_limit(value);
            if (toggle->query() == 0.f)
                toggle->set(1);
        }

        command cmd_get(ds::AUTO_CALIB);
        cmd_get.param1 = 5;
        std::vector<uint8_t> ret = _hwm.send(cmd_get);
        if (ret.empty())
            throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

        command cmd(ds::AUTO_CALIB);
        cmd.param1 = 4;
        cmd.param2 = static_cast<int>(value);
        cmd.param3 = *(reinterpret_cast<uint32_t*>(ret.data() + 4));
        _hwm.send(cmd);
        _record_action(*this);
    }

    float auto_exposure_limit_option::query() const
    {
        command cmd(ds::AUTO_CALIB);
        cmd.param1 = 5;

        auto res = _hwm.send(cmd);
        if (res.empty())
            throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

        auto ret = static_cast<float>(*(reinterpret_cast<uint32_t*>(res.data())));
        if (ret< get_range().min || ret > get_range().max)
        {
            if (auto toggle = _exposure_limit_toggle.lock())
                return toggle->get_cached_limit();
        }
        return ret;
    }

    option_range auto_exposure_limit_option::get_range() const
    {
        return *_range;
    }

    auto_gain_limit_option::auto_gain_limit_option(hw_monitor& hwm, sensor_base* ep, option_range range, std::shared_ptr <limits_option> gain_limit_enable)
        : option_base(range), _hwm(hwm), _sensor(ep), _gain_limit_toggle(gain_limit_enable)
    {
        _range = [range]()
        {
            return range;
        };
        if (auto toggle = _gain_limit_toggle.lock())
            toggle->set_cached_limit(range.max);
    }

    void auto_gain_limit_option::set(float value)
    {
        if (!is_valid(value))
            throw invalid_value_exception("set(enable_auto_gain) failed! Invalid Auto-Gain mode request " + std::to_string(value));

        if (auto toggle = _gain_limit_toggle.lock())
        {
            toggle->set_cached_limit(value);
            if (toggle->query() == 0.f)
                toggle->set(1);
        }
            

        command cmd_get(ds::AUTO_CALIB);
        cmd_get.param1 = 5;
        std::vector<uint8_t> ret = _hwm.send(cmd_get);
        if (ret.empty())
            throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

        command cmd(ds::AUTO_CALIB);
        cmd.param1 = 4;
        cmd.param2 = *(reinterpret_cast<uint32_t*>(ret.data()));
        cmd.param3 = static_cast<int>(value);
        _hwm.send(cmd);
        _record_action(*this);
    }

    float auto_gain_limit_option::query() const
    {
        command cmd(ds::AUTO_CALIB);
        cmd.param1 = 5;

        auto res = _hwm.send(cmd);
        if (res.empty())
            throw invalid_value_exception("auto_exposure_limit_option::query result is empty!");

        auto ret = static_cast<float>(*(reinterpret_cast<uint32_t*>(res.data() + 4)));
        if (ret< get_range().min || ret > get_range().max)
        {
            if (auto toggle = _gain_limit_toggle.lock())
                return toggle->get_cached_limit();
        }
        return ret;
    }

    option_range auto_gain_limit_option::get_range() const
    {
        return *_range;
    }

    librealsense::thermal_compensation::thermal_compensation(
        std::shared_ptr<ds5_thermal_monitor> monitor,
        std::shared_ptr<option> toggle) :
        _thermal_monitor(monitor),
        _thermal_toggle(toggle)
    {
    }

    float librealsense::thermal_compensation::query(void) const
    {
        auto val = _thermal_toggle->query();
        return val;
    }

    void librealsense::thermal_compensation::set(float value)
    {
        if (value < 0)
            throw invalid_value_exception("Invalid input for thermal compensation toggle: " + std::to_string(value));

        _thermal_toggle->set(value);
        _recording_function(*this);
    }

    const char* librealsense::thermal_compensation::get_description() const
    {
        return "Toggle thermal compensation adjustments mechanism";
    }

    const char* librealsense::thermal_compensation::get_value_description(float value) const
    {
        if (value == 0)
        {
            return "Thermal compensation is disabled";
        }
        else
        {
            return "Thermal compensation is enabled";
        }
    }

    //Work-around the control latency
    void librealsense::thermal_compensation::create_snapshot(std::shared_ptr<option>& snapshot) const
    {
        snapshot = std::make_shared<const_value_option>(get_description(), 0.f);
    }
}
