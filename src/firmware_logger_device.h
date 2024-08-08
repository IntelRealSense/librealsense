// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-2024 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"
#include "device.h"
#include "hw-monitor.h"
#include "fw-logs/fw-log-data.h"
#include "fw-logs/fw-logs-parser.h"

#include <map>
#include <string>

namespace librealsense
{
    class firmware_logger_extensions
    {
    public:
        virtual void start() = 0;
        virtual void stop() = 0;
        virtual bool get_fw_log( fw_logs::fw_logs_binary_data & binary_data ) = 0;
        virtual bool get_flash_log( fw_logs::fw_logs_binary_data & binary_data ) = 0;
        virtual unsigned int get_number_of_fw_logs() const = 0;
        virtual bool init_parser( const std::string & xml_content ) = 0;
        virtual bool parse_log( const fw_logs::fw_logs_binary_data * fw_log_msg, fw_logs::fw_log_data * parsed_msg ) = 0;
        virtual ~firmware_logger_extensions() = default;
    };
    MAP_EXTENSION( RS2_EXTENSION_FW_LOGGER, librealsense::firmware_logger_extensions );

    class firmware_logger_device : public virtual device, public firmware_logger_extensions
    {
    public:
        firmware_logger_device( std::shared_ptr< const device_info > const & dev_info,
                                std::shared_ptr< hw_monitor > hardware_monitor,
                                const command & fw_logs_command,
                                const command & flash_logs_command );

        // Legacy devices always collecting, no need to start/stop
        void start() override { return; }
        void stop() override { return; }

        bool get_fw_log( fw_logs::fw_logs_binary_data & binary_data ) override;
        bool get_flash_log( fw_logs::fw_logs_binary_data & binary_data ) override;
        unsigned int get_number_of_fw_logs() const override;

        bool init_parser( const std::string & xml_content ) override;
        bool parse_log( const fw_logs::fw_logs_binary_data * fw_log_msg, fw_logs::fw_log_data * parsed_msg ) override;

    protected:
        void get_fw_logs_from_hw_monitor();
        void handle_received_data( const std::vector< uint8_t > & res );
        void get_flash_logs_from_hw_monitor();
        virtual command get_update_command();
        virtual size_t get_log_size( const uint8_t * buff ) const;
        
        command _fw_logs_command;
        std::shared_ptr< hw_monitor > _hw_monitor;
        std::queue< fw_logs::fw_logs_binary_data > _fw_logs;
        std::unique_ptr< fw_logs::fw_logs_parser > _parser;

        command _flash_logs_command;
        std::queue< fw_logs::fw_logs_binary_data > _flash_logs;
        bool _flash_logs_initialized;
    };

    class extended_firmware_logger_device : public firmware_logger_device
    {
    public:
        extended_firmware_logger_device( std::shared_ptr< const device_info > const & dev_info,
                                         std::shared_ptr< hw_monitor > hardware_monitor,
                                         const command & fw_logs_command );

        void start() override;
        void stop() override;

        bool get_flash_log( fw_logs::fw_logs_binary_data & binary_data ) override { return false; } // Not supported

        bool init_parser( const std::string & xml_content ) override;

        void set_expected_source_versions( std::map< int, std::string > && expected_versions )
        {
            _source_to_expected_version = std::move( expected_versions );
        }

    protected:
        command get_update_command() override;
        size_t get_log_size( const uint8_t * buff ) const override;

        std::map< int, std::string > _source_to_expected_version;
    };
}
