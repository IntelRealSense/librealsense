// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class safety_sensor
    {
    public:
        virtual ~safety_sensor() = default;

        virtual void set_safety_preset(int index, const rs2_safety_preset& sp) const = 0;
        virtual rs2_safety_preset get_safety_preset(int index) const = 0;
        virtual void set_safety_interface_config(const rs2_safety_interface_config& sic) const = 0;
        virtual rs2_safety_interface_config get_safety_interface_config(rs2_calib_location loc) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_SAFETY_SENSOR, librealsense::safety_sensor);
}
