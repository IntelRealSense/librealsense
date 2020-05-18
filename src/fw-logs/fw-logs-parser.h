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
            fw_log_data parse_fw_log(uint32_t event_id, uint32_t p1, uint32_t p2, uint32_t p3, 
                uint32_t file_id, uint32_t thread_id);
            

        private:
            void fill_log_data(const char* fw_logs, fw_log_data* log_data);

            fw_logs_formating_options _fw_logs_formating_options;
        };
    }
}
