// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include "ds/ds-private.h"
#include "core/options.h"

#pragma once

namespace librealsense
{
    class rgb_tnr_option : public option
    {
    public:
        rgb_tnr_option(std::shared_ptr<hw_monitor> hwm, sensor_base* ep);
        virtual ~rgb_tnr_option() = default;
        virtual void set(float value) override;
        virtual float query() const override;
        virtual option_range get_range() const override;
        virtual bool is_enabled() const override { return true; }
        virtual const char* get_description() const override
        {
            return "RGB TNR: 0:disabled(default), 1:enabled. Can only be set before streaming";
        }
        virtual void enable_recording(std::function<void(const option&)> record_action) override { _record_action = record_action; }

        static int const GET_TNR_STATE = 0;
        static int const SET_TNR_STATE = 1;

    private:
        std::function<void(const option&)> _record_action = [](const option&) {};
        lazy<option_range> _range;
        std::shared_ptr<hw_monitor> _hwm;
        sensor_base* _sensor;
    };
}
