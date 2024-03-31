/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2024 Intel Corporation. All Rights Reserved. */

#pragma once

#include <src/fw-logs/fw-log-data.h>
#include <src/fw-logs/fw-string-formatter.h>
#include <src/fw-logs/fw-logs-formatting-options.h>

#include <src/hw-monitor.h>

#include <string>
#include <vector>
#include <memory>

namespace librealsense
{
    namespace fw_logs
    {
        class fw_logs_parser : public std::enable_shared_from_this< fw_logs_parser >
        {
        public:
            explicit fw_logs_parser( const std::string & definitions_xml );
            virtual ~fw_logs_parser() = default;

            fw_log_data parse_fw_log( const fw_logs_binary_data * fw_log_msg );
            virtual size_t get_log_size( const uint8_t * log ) const;

        protected:
            struct structured_binary_data  // Common format for legacy and extended binary data formats
            {
                uint32_t severity = 0;
                uint32_t source_id = 0;
                uint32_t file_id = 0;
                uint32_t module_id = 0;
                uint32_t event_id = 0;
                uint32_t line = 0;
                uint32_t sequence = 0;
                uint64_t timestamp = 0;

                std::vector< fw_logs::param_info > params_info;
                std::vector< uint8_t > params_blob;
            };

            void initialize_source_formatting_options( const std::pair< const int, std::string > & source,
                                                       const std::string & definitions_xml );

            structured_binary_data structure_common_data( const fw_logs_binary_data * fw_log_msg ) const;

            virtual void structure_timestamp( const fw_logs_binary_data * raw,
                                              structured_binary_data * structured ) const;
            virtual void structure_params( const fw_logs_binary_data * raw,
                                           size_t num_of_params,
                                           structured_binary_data * structured ) const;

            virtual const fw_logs_formatting_options & get_format_options_ref( int source_id ) const;
            virtual std::string get_source_name( int source_id ) const;
            virtual rs2_log_severity parse_severity( uint32_t severity ) const;
            
            std::map< int, fw_logs_formatting_options > _source_to_formatting_options;
            std::map< int, std::string > _source_id_to_name;
        };

        class extended_fw_logs_parser: public fw_logs_parser
        {
        public:
            explicit extended_fw_logs_parser( const std::string & definitions_xml,
                                              const std::map< int, std::string > & expected_versions = {} );

            size_t get_log_size( const uint8_t * log ) const override;

            command get_start_command() const;
            command get_update_command() const;
            command get_stop_command() const;

        protected:
            void initialize_source_verbosity_settings( const std::pair< const int, std::string > & source,
                                                       const std::string & definitions_xml );
            void validate_source_version( int source_id,
                                          const std::string & expected_version, // Throws if not as expected
                                          const std::string & definitions_xml );

            void structure_timestamp( const fw_logs_binary_data * raw,
                                      structured_binary_data * structured ) const override;
            void structure_params( const fw_logs_binary_data * raw,
                                   size_t num_of_params,
                                   structured_binary_data * structured ) const override;

            const fw_logs_formatting_options & get_format_options_ref( int source_id ) const override;
            std::string get_source_name( int source_id ) const override;
            rs2_log_severity parse_severity( uint32_t severity ) const override;

            fw_logs::extended_log_request _verbosity_settings;
        };
    }
}
