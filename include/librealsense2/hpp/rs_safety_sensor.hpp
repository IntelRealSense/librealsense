// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SAFETY_SENSOR_HPP
#define LIBREALSENSE_RS2_SAFETY_SENSOR_HPP

#include "rs_sensor.hpp"
#include "rs_safety_types.hpp"

namespace rs2
{
    class safety_sensor : public sensor
    {
    public:
        safety_sensor(sensor s)
            : sensor(s.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_sensor_extendable_to(_sensor.get(), RS2_EXTENSION_SAFETY_SENSOR, &e) == 0 && !e)
            {
                _sensor.reset();
            }
            error::handle(e);
        }

        operator bool() const { return _sensor.get() != nullptr; }

        rs2_safety_preset get_safety_preset(int index) const
        {
            rs2_error* e = nullptr;
            rs2_safety_preset sp;
            rs2_get_safety_preset(_sensor.get(), index, &sp, &e);
            error::handle(e);
            return sp;
        }

        void set_safety_preset(int index, rs2_safety_preset const& sp) const
        {
            rs2_error* e = nullptr;
            rs2_set_safety_preset(_sensor.get(), index, &sp, &e);
            error::handle(e);
        }

        rs2_safety_preset json_string_to_safety_preset(const std::string& json_str) const
        {
            rs2_error* e = nullptr;
            rs2_safety_preset sp;
            rs2_json_string_to_safety_preset(_sensor.get(), json_str.c_str(), &sp, &e);
            error::handle(e);
            return sp;
        }

        std::string safety_preset_to_json_string(rs2_safety_preset const& sp) const
        {
            rs2_error* e = nullptr;
            std::string json_str = rs2_safety_preset_to_json_string(_sensor.get(), &sp, &e);
            error::handle(e);
            return json_str;
        }

        rs2_safety_interface_config get_safety_interface_config(rs2_calib_location calib_location = RS2_CALIB_LOCATION_RAM) const
        {
            rs2_error* e = nullptr;
            rs2_safety_interface_config sic;
            rs2_get_safety_interface_config(_sensor.get(), &sic, calib_location, &e);
            error::handle(e);
            return sic;
        }

        void set_safety_interface_config(rs2_safety_interface_config const& sic) const
        {
            rs2_error* e = nullptr;
            rs2_set_safety_interface_config(_sensor.get(), &sic, &e);
            error::handle(e);
        }

        rs2_safety_interface_config json_string_to_safety_interface_config(const std::string& json_str) const
        {
            rs2_error* e = nullptr;
            rs2_safety_interface_config sic;
            rs2_json_string_to_safety_interface_config(_sensor.get(), json_str.c_str(), &sic, &e);
            error::handle(e);
            return sic;
        }

        std::string safety_interface_config_to_json_string(rs2_safety_interface_config const& sic) const
        {
            rs2_error* e = nullptr;
            std::string json_str = rs2_safety_interface_config_to_json_string(_sensor.get(), &sic, &e);
            error::handle(e);
            return json_str;
        }
    };
}
#endif // LIBREALSENSE_RS2_SAFETY_SENSOR_HPP
