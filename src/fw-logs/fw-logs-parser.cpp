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
        fw_logs_parser::fw_logs_parser(string xml_content)
            : _fw_logs_formating_options(xml_content),
            _last_timestamp(0),
            _timestamp_factor(0.00001)
        {
            _fw_logs_formating_options.initialize_from_xml();
        }


        fw_logs_parser::~fw_logs_parser(void)
        {
        }

        fw_log_data fw_logs_parser::parse_fw_log(const fw_logs_binary_data* fw_log_msg) 
        {
            fw_log_data log_data;

            if (!fw_log_msg || fw_log_msg->logs_buffer.size() == 0)
                return log_data;

            log_data = fill_log_data(fw_log_msg);

            //message
            fw_string_formatter reg_exp(_fw_logs_formating_options.get_enums());
            fw_log_event log_event_data;
            _fw_logs_formating_options.get_event_data(log_data._event_id, &log_event_data);

            uint32_t params[3] = { log_data._p1, log_data._p2, log_data._p3 };
            reg_exp.generate_message(log_event_data.line, log_event_data.num_of_params, params, &log_data._message);

            //file_name
            _fw_logs_formating_options.get_file_name(log_data._file_id, &log_data._file_name);

            //thread_name
            _fw_logs_formating_options.get_thread_name(log_data._thread_id, &log_data._thread_name);

            return log_data;
        }

        fw_log_data fw_logs_parser::fill_log_data(const fw_logs_binary_data* fw_log_msg)
        {
            fw_log_data log_data;

            auto* log_binary = reinterpret_cast<const fw_logs::fw_log_binary*>(fw_log_msg->logs_buffer.data());

            //parse first DWORD
            log_data._magic_number = static_cast<uint32_t>(log_binary->dword1.bits.magic_number);
            log_data._severity = static_cast<uint32_t>(log_binary->dword1.bits.severity);
            log_data._thread_id = static_cast<uint32_t>(log_binary->dword1.bits.thread_id);
            log_data._file_id = static_cast<uint32_t>(log_binary->dword1.bits.file_id);
            log_data._group_id = static_cast<uint32_t>(log_binary->dword1.bits.group_id);

            //parse second DWORD
            log_data._event_id = static_cast<uint32_t>(log_binary->dword2.bits.event_id);
            log_data._line = static_cast<uint32_t>(log_binary->dword2.bits.line_id);
            log_data._sequence = static_cast<uint32_t>(log_binary->dword2.bits.seq_id);

            //parse third DWORD
            log_data._p1 = static_cast<uint32_t>(log_binary->dword3.p1);
            log_data._p2 = static_cast<uint32_t>(log_binary->dword3.p2);

            //parse forth DWORD
            log_data._p3 = static_cast<uint32_t>(log_binary->dword4.p3);

            //parse fifth DWORD
            log_data._timestamp = log_binary->dword5.timestamp;

            log_data._delta = (_last_timestamp == 0) ? 
                0 :(log_data._timestamp - _last_timestamp) * _timestamp_factor;

            _last_timestamp = log_data._timestamp;

            return log_data;
        }
    }
}
