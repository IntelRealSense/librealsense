/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once
#include <string>
#include <vector>
#include <memory>
#include "fw-logs-formating-options.h"
#include "fw-log-data.h"

namespace librealsense
{
    namespace fw_logs
    {
        class fw_logs_parser : public std::enable_shared_from_this<fw_logs_parser>
        {
        public:
            explicit fw_logs_parser( const std::string & xml_content);
            virtual ~fw_logs_parser() = default;

            fw_log_data parse_fw_log(const fw_logs_binary_data* fw_log_msg);

        protected:
            struct structured_binary_data // Common format for legacy and extended binary data formats
            {
                uint32_t severity = 0;
                uint32_t source_id = 0;
                uint32_t file_id = 0;
                uint32_t module_id = 0;
                uint32_t event_id = 0;
                uint32_t line = 0;
                uint32_t sequence = 0;
                uint64_t timestamp = 0;

                std::vector< param_info > params_info;
                std::vector< uint8_t > params_blob;
            };

            structured_binary_data structure_common_data( const fw_logs_binary_data * fw_log_msg ) const;
            virtual void structure_timestamp( const fw_logs_binary_data * raw,
                                              structured_binary_data * structured ) const;
            virtual void structure_params( const fw_logs_binary_data * raw,
                                           size_t num_of_params,
                                           structured_binary_data * structured ) const;

        private:
            fw_logs_formating_options _fw_logs_formating_options;
        };

        class legacy_fw_logs_parser : public fw_logs_parser
        {
        public:
            explicit legacy_fw_logs_parser( const std::string & xml_content );

        protected:
            void structure_timestamp( const fw_logs_binary_data * raw,
                                      structured_binary_data * structured ) const override;
            void structure_params( const fw_logs_binary_data * raw,
                                   size_t num_of_params,
                                   structured_binary_data * structured ) const override;
        };
    }
}
