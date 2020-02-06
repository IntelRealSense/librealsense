// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include "fw-logs-formating-options.h"
#include "fw-logs-xml-helper.h"
#include <sstream>


using namespace std;

namespace fw_logger
{
    fw_log_event::fw_log_event()
        : num_of_params(0),
          line("")
    {}

    fw_log_event::fw_log_event(int input_num_of_params, const string& input_line)
        : num_of_params(input_num_of_params),
          line(input_line)
    {}


    fw_logs_formating_options::fw_logs_formating_options(const string& xml_full_file_path)
        : _xml_full_file_path(xml_full_file_path)
    {}


    fw_logs_formating_options::~fw_logs_formating_options(void)
    {
    }

    bool fw_logs_formating_options::get_event_data(int id, fw_log_event* log_event_data) const
    {
        auto event_it = _fw_logs_event_list.find(id);
        if (event_it != _fw_logs_event_list.end())
        {
            *log_event_data = event_it->second;
            return true;
        }
        else
        {
            stringstream ss;
            ss << "*** Unrecognized Log Id: ";
            ss << id;
            ss << "! P1 = 0x{0:x}, P2 = 0x{1:x}, P3 = 0x{2:x}";
            log_event_data->line = ss.str();
            log_event_data->num_of_params = 3;
            return false;
        }
    }

    bool fw_logs_formating_options::get_file_name(int id, string* file_name) const
    {
        auto file_it = _fw_logs_file_names_list.find(id);
        if (file_it != _fw_logs_file_names_list.end())
        {
            *file_name = file_it->second;
            return true;
        }
        else
        {
            *file_name = "Unknown";
            return false;
        }
    }

    bool fw_logs_formating_options::get_thread_name(uint32_t thread_id, string* thread_name) const
    {
        auto file_it = _fw_logs_thread_names_list.find(thread_id);
        if (file_it != _fw_logs_thread_names_list.end())
        {
            *thread_name = file_it->second;
            return true;
        }
        else
        {
            *thread_name = "Unknown";
            return false;
        }
    }

    std::unordered_map<string, std::vector<kvp>> fw_logs_formating_options::get_enums() const
    {
        return _fw_logs_enum_names_list;
    }

    bool fw_logs_formating_options::initialize_from_xml()
    {
        fw_logs_xml_helper fw_logs_xml(_xml_full_file_path);
        return fw_logs_xml.build_log_meta_data(this);
    }
}
