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
}
