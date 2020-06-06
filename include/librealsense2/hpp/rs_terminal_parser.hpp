// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_TERMINAL_HPP
#define LIBREALSENSE_RS2_TERMINAL_HPP

#include "rs_types.hpp"
#include "rs_sensor.hpp"
#include "rs_device.hpp"
#include "../h/rs_firmware_logs.h"

#include <string>
#include <vector>

namespace rs2
{
    class terminal_parser
    {
    public:
        terminal_parser(const std::string& xml_path)
        {
            rs2_error* e = nullptr;
            

            _terminal_parser = std::shared_ptr<rs2_terminal_parser>(
                rs2_create_terminal_parser(xml_path.c_str(),  &e),
                rs2_delete_terminal_parser);
            error::handle(e);            
        }

        std::vector<uint8_t> parse_command(const rs2::device_list& dev_list, const std::string& command)
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_device> dev(
                rs2_create_device(dev_list.get_list(), 0, &e),
                rs2_delete_device);
            error::handle(e);

            std::shared_ptr<const rs2_raw_data_buffer> list(
                rs2_terminal_parse_command(_terminal_parser.get(), dev.get(), command.c_str(), command.size(), &e),
                rs2_delete_raw_data);
            error::handle(e);

            auto size = rs2_get_raw_data_size(list.get(), &e);
            error::handle(e);

            auto start = rs2_get_raw_data(list.get(), &e);

            std::vector<uint8_t> results;
            results.insert(results.begin(), start, start + size);

            return results;
        }

    private:
        std::shared_ptr<rs2_terminal_parser> _terminal_parser;
    };
}


#endif // LIBREALSENSE_RS2_TYPES_HPP
