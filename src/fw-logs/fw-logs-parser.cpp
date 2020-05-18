// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "fw-logs-parser.h"
#include <regex>
#include <sstream>
#include "fw-string-formatter.h"
#include "stdint.h"

using namespace std;

namespace librealsense
{
    namespace fw_logs
    {
        fw_logs_parser::fw_logs_parser(string xml_full_file_path)
            : _fw_logs_formating_options(xml_full_file_path)
        {
            _fw_logs_formating_options.initialize_from_xml();
        }


        fw_logs_parser::~fw_logs_parser(void)
        {
        }

        fw_log_data fw_logs_parser::parse_fw_log(uint32_t event_id, uint32_t p1, uint32_t p2, uint32_t p3, 
            uint32_t file_id, uint32_t thread_id)
        {
            fw_log_data result;

            fw_string_formatter reg_exp(_fw_logs_formating_options.get_enums());
            fw_log_event log_event_data;

            _fw_logs_formating_options.get_event_data(event_id, &log_event_data);
            uint32_t params[3] = { p1, p2, p3 };
            reg_exp.generate_message(log_event_data.line, log_event_data.num_of_params, params, &result.message);

            _fw_logs_formating_options.get_file_name(file_id, &result.file_name);
            _fw_logs_formating_options.get_thread_name(static_cast<uint32_t>(thread_id), &result.thread_name);

            return result;
        }
    }
}
