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

        void fw_logs_parser::parse_fw_log(rs2_firmware_log_message** fw_log_msg)
        {
            /*if (!fw_log_msg || !(*fw_log_msg))
                return;
            //message
            fw_string_formatter reg_exp(_fw_logs_formating_options.get_enums());
            fw_log_event log_event_data;
            _fw_logs_formating_options.get_event_data((*fw_log_msg)->_event_id, &log_event_data);

            uint32_t params[3] = { (*fw_log_msg)->_p1, (*fw_log_msg)->_p2, (*fw_log_msg)->_p3 };
            std::string parsed_message;
            reg_exp.generate_message(log_event_data.line, log_event_data.num_of_params, params, &parsed_message);
            //reallocate char pointer and copy string into it
            /*if ((*fw_log_msg)->_message)
            {
                delete[] (*fw_log_msg)->_message;
            }*/
            /*(*fw_log_msg)->_message = new char[parsed_message.size() + 1];
            strncpy_s((*fw_log_msg)->_message, parsed_message.size() + 1, parsed_message.c_str(), parsed_message.size());

            //file name
            std::string parsed_file_name;
            _fw_logs_formating_options.get_file_name((*fw_log_msg)->_file_id, &parsed_file_name);
            //reallocate char pointer and copy string into it
            /*if ((*fw_log_msg)->_file_name)
            {
                delete (*fw_log_msg)->_file_name;
            }*/
            /*(*fw_log_msg)->_file_name = new char[parsed_file_name.size() + 1];
            strncpy_s((*fw_log_msg)->_file_name, parsed_file_name.size() + 1, parsed_file_name.c_str(), parsed_file_name.size());

            //thread name
            std::string parsed_thread_name;
            _fw_logs_formating_options.get_thread_name(static_cast<uint32_t>((*fw_log_msg)->_thread_id), &parsed_thread_name);
            //reallocate char pointer and copy string into it
            /*if ((*fw_log_msg)->_thread_name)
            {
                delete (*fw_log_msg)->_thread_name;
            }*/
            /*(*fw_log_msg)->_thread_name = new char[parsed_thread_name.size() + 1];
            strncpy_s((*fw_log_msg)->_thread_name, parsed_thread_name.size() + 1, parsed_thread_name.c_str(), parsed_thread_name.size());*/
        }
    }
}
