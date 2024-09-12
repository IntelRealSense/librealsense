// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"
#include "device.h"
#include "hw-monitor.h"
#include <vector>
#include "fw-logs/fw-log-data.h"
#include "fw-logs/fw-logs-parser.h"

namespace librealsense
{
    class firmware_logger_extensions
    {
    public:
        virtual bool get_fw_log(fw_logs::fw_logs_binary_data& binary_data) = 0;
        virtual bool get_flash_log(fw_logs::fw_logs_binary_data& binary_data) = 0;
        virtual unsigned int get_number_of_fw_logs() const = 0;
        virtual bool init_parser(std::string xml_content) = 0;
        virtual bool parse_log(const fw_logs::fw_logs_binary_data* fw_log_msg, fw_logs::fw_log_data* parsed_msg) = 0;
        virtual ~firmware_logger_extensions() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_FW_LOGGER, librealsense::firmware_logger_extensions);

    class firmware_logger_device : public virtual device, public firmware_logger_extensions
    {
    public:
        firmware_logger_device( std::shared_ptr< const device_info > const & dev_info,
            std::shared_ptr<hw_monitor> hardware_monitor,
            const command& fw_logs_command, const command& flash_logs_command);

        bool get_fw_log(fw_logs::fw_logs_binary_data& binary_data) override;
        bool get_flash_log(fw_logs::fw_logs_binary_data& binary_data) override;
        
        unsigned int get_number_of_fw_logs() const override;

        bool init_parser(std::string xml_content) override;
        bool parse_log(const fw_logs::fw_logs_binary_data* fw_log_msg, fw_logs::fw_log_data* parsed_msg) override;

        // Temporal solution for HW_Monitor injection
        void assign_hw_monitor(std::shared_ptr<hw_monitor> hardware_monitor)
            { _hw_monitor = hardware_monitor; }

    private:

        void get_fw_logs_from_hw_monitor();
        void get_flash_logs_from_hw_monitor();

        command _fw_logs_command;
        command _flash_logs_command;

        std::shared_ptr<hw_monitor> _hw_monitor;

        std::queue<fw_logs::fw_logs_binary_data> _fw_logs;
        std::queue<fw_logs::fw_logs_binary_data> _flash_logs;

        bool _flash_logs_initialized;

        fw_logs::fw_logs_parser* _parser;
        uint16_t _device_pid;

    };

}
