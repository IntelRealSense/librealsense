// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.


#include "d500-options.h"

namespace librealsense
{
    rgb_tnr_option::rgb_tnr_option(std::shared_ptr<hw_monitor> hwm, sensor_base* ep)
        : _hwm(hwm), _sensor(ep)
    {
        _range = [this]()
        {
            return option_range{ 0, 1, 1, 0 };
        };
    }

    void rgb_tnr_option::set(float value)
    {
        if (_sensor->is_streaming())
            throw std::runtime_error("Cannot change RGB TNR option while streaming!");

        command cmd(ds::RGB_TNR);
        cmd.param1 = SET_TNR_STATE;
        cmd.param2 = static_cast<int>(value);

        _hwm->send(cmd);
        _record_action(*this);
    }

    float rgb_tnr_option::query() const
    {
        command cmd(ds::RGB_TNR);
        cmd.param1 = GET_TNR_STATE;

        auto res = _hwm->send(cmd);
        if (res.empty())
            throw invalid_value_exception("rgb_tnr_option::query result is empty!");

        return (res.front());
    }

    option_range rgb_tnr_option::get_range() const
    {
        return *_range;
    }

    temperature_option::temperature_option(std::shared_ptr<hw_monitor> hwm, sensor_base* ep, 
        temperature_component component, const char* description)
        : _hwm(hwm), _sensor(ep), _component(component), _description(description)
    {
        _range = [this]()
        {
            return option_range{ -127, 128, 1, 30 };
        };
    }

    float temperature_option::query() const
    {
        if (!is_enabled() || !_hwm)
            throw wrong_api_call_sequence_exception("error occurred in the temperature reading");

        float temperature = -1;
        try {
            command cmd(ds::fw_cmd::GTEMP, static_cast<int>(_component));
            auto res = _hwm->send(cmd);
            auto offset_for_component = 2 * (static_cast<int>(_component) - 1);
            int8_t whole_number_part = static_cast<int8_t>(res[offset_for_component]);
            int8_t decimal_part = static_cast<int8_t>(res[offset_for_component + 1]);
            temperature = static_cast<float>(whole_number_part) + decimal_part / 256.f;
        }
        catch (...)
        {
            throw wrong_api_call_sequence_exception("hw monitor command for reading temperature failed");
        }

        return temperature;
    }
}
