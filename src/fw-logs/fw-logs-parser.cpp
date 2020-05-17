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
            : _fw_logs_formating_options(xml_full_file_path),
            _last_timestamp(0),
            _timestamp_factor(0.00001)
        {
            _fw_logs_formating_options.initialize_from_xml();
        }


        fw_logs_parser::~fw_logs_parser(void)
        {
        }

        vector<string> fw_logs_parser::get_fw_log_lines(const fw_logs_binary_data& fw_logs_data_binary)
        {
            fw_logs_binary_data binary_data = { fw_logs_data_binary };
            vector<string> string_vector;
            int num_of_lines = int(binary_data.logs_buffer.size()) / sizeof(fw_log_binary);
            auto temp_pointer = reinterpret_cast<fw_log_binary const*>(binary_data.logs_buffer.data());

            for (int i = 0; i < num_of_lines; i++)
            {
                string line;
                auto log = const_cast<char*>(reinterpret_cast<char const*>(temp_pointer));
                line = generate_log_line(log);
                string_vector.push_back(line);
                temp_pointer++;
            }
            return string_vector;
        }

        string fw_logs_parser::generate_log_line(char* fw_logs)
        {
            fw_log_data log_data;
            fill_log_data(fw_logs, &log_data);

            return log_data.to_string();
        }

        fw_log_data fw_logs_parser::generate_log_data(char* fw_logs)
        {
            fw_log_data log_data;
            fill_log_data(fw_logs, &log_data);

            return log_data;
        }

        vector<fw_log_data> fw_logs_parser::get_fw_log_data(const fw_logs_binary_data& fw_logs_data_binary)
        {
            fw_logs_binary_data binary_data = { fw_logs_data_binary };
            vector<fw_log_data> data_vector;
            int num_of_lines = int(binary_data.logs_buffer.size()) / sizeof(fw_log_binary);
            auto temp_pointer = reinterpret_cast<fw_log_binary const*>(binary_data.logs_buffer.data());

            for (int i = 0; i < num_of_lines; i++)
            {
                fw_log_data data;
                auto log = const_cast<char*>(reinterpret_cast<char const*>(temp_pointer));
                data = generate_log_data(log);
                data_vector.push_back(data);
                temp_pointer++;
            }
            return data_vector;
        }

        void fw_logs_parser::fill_log_data(const char* fw_logs, fw_log_data* log_data)
        {
            fw_string_formatter reg_exp(_fw_logs_formating_options.get_enums());
            fw_log_event log_event_data;

            auto* log_binary = reinterpret_cast<const fw_log_binary*>(fw_logs);

            //parse first DWORD
            log_data->magic_number = static_cast<uint32_t>(log_binary->dword1.bits.magic_number);
            log_data->severity = static_cast<uint32_t>(log_binary->dword1.bits.severity);

            log_data->file_id = static_cast<uint32_t>(log_binary->dword1.bits.file_id);
            log_data->group_id = static_cast<uint32_t>(log_binary->dword1.bits.group_id);

            //parse second DWORD
            log_data->event_id = static_cast<uint32_t>(log_binary->dword2.bits.event_id);
            log_data->line = static_cast<uint32_t>(log_binary->dword2.bits.line_id);
            log_data->sequence = static_cast<uint32_t>(log_binary->dword2.bits.seq_id);

            //parse third DWORD
            log_data->p1 = static_cast<uint32_t>(log_binary->dword3.p1);
            log_data->p2 = static_cast<uint32_t>(log_binary->dword3.p2);

            //parse forth DWORD
            log_data->p3 = static_cast<uint32_t>(log_binary->dword4.p3);

            //parse fifth DWORD
            log_data->timestamp = log_binary->dword5.timestamp;

            if (_last_timestamp == 0)
            {
                log_data->delta = 0;
            }
            else
            {
                log_data->delta = (log_data->timestamp - _last_timestamp) * _timestamp_factor;
            }

            _last_timestamp = log_data->timestamp;
            _fw_logs_formating_options.get_event_data(log_data->event_id, &log_event_data);
            uint32_t params[3] = { log_data->p1, log_data->p2, log_data->p3 };
            reg_exp.generate_message(log_event_data.line, log_event_data.num_of_params, params, &log_data->message);

            _fw_logs_formating_options.get_file_name(log_data->file_id, &log_data->file_name);
            _fw_logs_formating_options.get_thread_name(static_cast<uint32_t>(log_binary->dword1.bits.thread_id),
                &log_data->thread_name);
        }
    }
}
