// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../include/librealsense/rs2.h"

namespace librealsense
{
    struct option_range
    {
        float min;
        float max;
        float step;
        float def;
    };

    class option //TODO: Ziv, public recordable<option>
    {
    public:
        virtual void set(float value) = 0;
        virtual float query() const = 0;
        virtual option_range get_range() const = 0;
        virtual bool is_enabled() const = 0;
        virtual bool is_read_only() const { return false; }

        virtual const char* get_description() const = 0;
        virtual const char* get_value_description(float) const { return nullptr; }

        virtual ~option() = default;
    };

    class options_interface//TODO: Ziv, public recordable<options_interface>
    {
    public:
        virtual option& get_option(rs2_option id) = 0;
        virtual const option& get_option(rs2_option id) const = 0;
        virtual bool supports_option(rs2_option id) const = 0;

        virtual ~options_interface() = default;
    };
}
