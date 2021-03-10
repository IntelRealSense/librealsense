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
            explicit fw_logs_parser(std::string xml_content);
            ~fw_logs_parser(void);

            fw_log_data parse_fw_log(const fw_logs_binary_data* fw_log_msg);


        private:
            fw_log_data fill_log_data(const fw_logs_binary_data* fw_log_msg);

            fw_logs_formating_options _fw_logs_formating_options;
            uint64_t _last_timestamp;
            const double _timestamp_factor;
        };
    }
}
