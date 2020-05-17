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
            explicit fw_logs_parser(std::string xml_full_file_path);
            ~fw_logs_parser(void);
            std::vector<std::string> get_fw_log_lines(const fw_logs_binary_data& fw_logs_data_binary);
            std::vector<fw_log_data> get_fw_log_data(const fw_logs_binary_data& fw_logs_data_binary);
            

        private:
            std::string generate_log_line(char* fw_logs);
            fw_log_data generate_log_data(char* fw_logs);
            void fill_log_data(const char* fw_logs, fw_log_data* log_data);
            uint64_t _last_timestamp;

            fw_logs_formating_options _fw_logs_formating_options;
            const double _timestamp_factor;
        };
    }
}
