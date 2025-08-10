// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2024 RealSense, Inc. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_SAFETY_SENSOR_HPP
#define LIBREALSENSE_RS2_SAFETY_SENSOR_HPP

#include "rs_sensor.hpp"

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

        std::string get_safety_preset(int index) const
        {
            std::vector<uint8_t> result;

            rs2_error* e = nullptr;
            auto buffer = rs2_get_safety_preset(_sensor.get(), index, &e);

            std::shared_ptr<const rs2_raw_data_buffer> list(buffer, rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);
            error::handle(e);

            result.insert(result.begin(), start, start + size);

            return std::string(result.begin(), result.end());
        }

        void set_safety_preset(int index, const std::string& sp_json_str) const
        {
            rs2_error* e = nullptr;
            rs2_set_safety_preset(_sensor.get(), index, sp_json_str.c_str(), &e);
            error::handle(e);
        }

        std::string get_safety_interface_config(rs2_calib_location calib_location = RS2_CALIB_LOCATION_RAM) const
        {
            std::vector<uint8_t> result;

            rs2_error* e = nullptr;
            auto buffer = rs2_get_safety_interface_config(_sensor.get(), calib_location, &e);

            std::shared_ptr<const rs2_raw_data_buffer> list(buffer, rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);
            error::handle(e);

            result.insert(result.begin(), start, start + size);

            return std::string(result.begin(), result.end());

        }

        void set_safety_interface_config(const std::string& sic_json_str) const
        {
            rs2_error* e = nullptr;
            rs2_set_safety_interface_config(_sensor.get(), sic_json_str.c_str(), &e);
            error::handle(e);

        }

        std::string get_application_config() const
        {
            std::vector<uint8_t> result;

            rs2_error* e = nullptr;
            auto buffer = rs2_get_application_config(_sensor.get(), &e);

            std::shared_ptr<const rs2_raw_data_buffer> list(buffer, rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);
            error::handle(e);

            result.insert(result.begin(), start, start + size);

            return std::string(result.begin(), result.end());
        }

        void set_application_config(const std::string& application_config_json_str) const
        {
            rs2_error* e = nullptr;
            rs2_set_application_config(_sensor.get(), application_config_json_str.c_str(), &e);
            error::handle(e);
        }

    };
}
#endif // LIBREALSENSE_RS2_SAFETY_SENSOR_HPP
