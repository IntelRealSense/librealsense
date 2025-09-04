// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023-2024 RealSense, Inc. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class safety_sensor
    {
    public:
        virtual ~safety_sensor() = default;
        virtual std::string get_safety_preset(int index) const = 0;
        virtual void set_safety_preset(int index, const std::string& sp_json_str) const = 0;
        virtual std::string get_safety_interface_config(rs2_calib_location loc) const = 0;
        virtual void set_safety_interface_config(const std::string& sic_json_str) const = 0;
        virtual std::string get_application_config() const = 0;
        virtual void set_application_config(const std::string& application_config_json_str) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_SAFETY_SENSOR, librealsense::safety_sensor);
}
