// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#ifndef LIBREALSENSE_RS2_FIRMWARE_LOGS_HPP
#define LIBREALSENSE_RS2_FIRMWARE_LOGS_HPP

#include "rs_types.hpp"
#include "rs_sensor.hpp"
#include "../h/rs_firmware_logs.h"
#include <array>

namespace rs2
{
    class firmware_log_message
    {
    public:
        explicit firmware_log_message(std::shared_ptr<rs2_firmware_log_message> msg):
        _fw_log_message(msg){}
        
        rs2_log_severity get_severity() const { 
            rs2_error* e = nullptr;
            rs2_log_severity severity = rs2_firmware_log_message_severity(_fw_log_message.get(), &e);
            error::handle(e);
            return severity;
        }
        std::string get_severity_str() const {
            return rs2_log_severity_to_string(get_severity());
        }

        uint32_t get_timestamp() const
        {
            rs2_error* e = nullptr;
            uint32_t timestamp = rs2_firmware_log_message_timestamp(_fw_log_message.get(), &e);
            error::handle(e);
            return timestamp;
        }

        int size() const 
        { 
            rs2_error* e = nullptr;
            int size = rs2_firmware_log_message_size(_fw_log_message.get(), &e);
            error::handle(e);
            return size;
        }
        
        std::vector<uint8_t> data() const 
        {
            rs2_error* e = nullptr;
            auto size = rs2_firmware_log_message_size(_fw_log_message.get(), &e);
            error::handle(e);
            std::vector<uint8_t> result;
            if (size > 0)
            {
                auto start = rs2_firmware_log_message_data(_fw_log_message.get(), &e);
                error::handle(e);
                result.insert(result.begin(), start, start + size);
            }
            return result;
        }

        const std::shared_ptr<rs2_firmware_log_message> get_message() const { return _fw_log_message; }

    private:        
        std::shared_ptr<rs2_firmware_log_message> _fw_log_message;
    };

    class firmware_log_parsed_message
    {
    public:
        explicit firmware_log_parsed_message(std::shared_ptr<rs2_firmware_log_parsed_message> msg) :
            _parsed_fw_log(msg) {}

        std::string message() const
        {
            rs2_error* e = nullptr;
            std::string msg(rs2_get_fw_log_parsed_message(_parsed_fw_log.get(), &e));
            error::handle(e);
            return msg;
        }
        std::string file_name() const
        {
            rs2_error* e = nullptr;
            std::string file_name(rs2_get_fw_log_parsed_file_name(_parsed_fw_log.get(), &e));
            error::handle(e);
            return file_name;
        }
        std::string thread_name() const
        {
            rs2_error* e = nullptr;
            std::string thread_name(rs2_get_fw_log_parsed_thread_name(_parsed_fw_log.get(), &e));
            error::handle(e);
            return thread_name;
        }
        std::string severity() const
        {
            rs2_error* e = nullptr;
            rs2_log_severity sev = rs2_get_fw_log_parsed_severity(_parsed_fw_log.get(), &e);
            error::handle(e);
            return std::string(rs2_log_severity_to_string(sev));
        }
        uint32_t line() const
        {
            rs2_error* e = nullptr;
            uint32_t line(rs2_get_fw_log_parsed_line(_parsed_fw_log.get(), &e));
            error::handle(e);
            return line;
        }
        uint32_t timestamp() const
        {
            rs2_error* e = nullptr;
            uint32_t timestamp(rs2_get_fw_log_parsed_timestamp(_parsed_fw_log.get(), &e));
            error::handle(e);
            return timestamp;
        }

    private:
        std::shared_ptr<rs2_firmware_log_parsed_message> _parsed_fw_log;
    };

    class firmware_logger : public device
    {
    public:
        firmware_logger(device d)
            : device(d.get())
        {
            rs2_error* e = nullptr;
            if (rs2_is_device_extendable_to(_dev.get(), RS2_EXTENSION_FW_LOGGER, &e) == 0 && !e)
            {
                _dev.reset();
            }
            error::handle(e);
        }

        rs2::firmware_log_message create_message()
        {
            rs2_error* e = nullptr;
            std::shared_ptr<rs2_firmware_log_message> msg(
                rs2_create_firmware_log_message(_dev.get(), &e),
                rs2_delete_firmware_log_message);
            error::handle(e);

            return firmware_log_message(msg);
        }

        bool get_firmware_log(rs2::firmware_log_message& msg) const
        {
            rs2_error* e = nullptr;
            rs2_firmware_log_message* m = msg.get_message().get();
            bool fw_log_pulling_status =
                rs2_get_firmware_log(_dev.get(), &(m), &e);

            error::handle(e);

            return fw_log_pulling_status;
        }

        bool get_flash_log(rs2::firmware_log_message& msg) const
        {
            rs2_error* e = nullptr;
            rs2_firmware_log_message* m = msg.get_message().get();
            bool flash_log_pulling_status =
                rs2_get_flash_log(_dev.get(), &(m), &e);

            error::handle(e);

            return flash_log_pulling_status;
        }

        int get_number_of_flash_logs() const
        {
            rs2_error* e = nullptr;
            int number_of_flash_logs =
                rs2_get_number_of_flash_logs(_dev.get(), &e);

            error::handle(e);

            return number_of_flash_logs;
        }

        bool init_parser(const std::string& xml_path)
        {
            rs2_error* e = nullptr;

            bool parser_initialized = rs2_init_parser(_dev.get(), xml_path.c_str(), &e);
            error::handle(e);

            return parser_initialized;
        }

        rs2::firmware_log_parsed_message parse_log(const rs2::firmware_log_message& msg)
        {
            rs2_error* e = nullptr;

            std::shared_ptr<rs2_firmware_log_parsed_message> parsed_msg(
                rs2_parse_firmware_log(_dev.get(), msg.get_message().get(), &e),
                rs2_delete_firmware_log_parsed_message);
            error::handle(e);

            return firmware_log_parsed_message(parsed_msg);
        }
    };

}
#endif // LIBREALSENSE_RS2_FIRMWARE_LOGS_HPP
