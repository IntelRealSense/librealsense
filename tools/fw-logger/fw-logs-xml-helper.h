/* License: Apache 2.0. See LICENSE file in root directory. */
/* Copyright(c) 2019 Intel Corporation. All Rights Reserved. */
#pragma once
#include "../../third-party/rapidxml/rapidxml_utils.hpp"
#include "fw-logs-formating-options.h"

using namespace rapidxml;

namespace fw_logger
{
    class fw_logs_xml_helper
    {
    public:
        enum node_type
        {
            event,
            file,
            thread,
            enums,
            none
        };

        fw_logs_xml_helper(std::string xml_full_file_path);
        ~fw_logs_xml_helper(void);

        bool build_log_meta_data(fw_logs_formating_options* logs_formating_options);

    private:
        bool init();
        bool build_meta_data_structure(xml_node<> *xml_node_list_of_events, fw_logs_formating_options* logs_formating_options);
        node_type get_next_node(xml_node<>* xml_node_list_of_events, int* id, int* num_of_params, std::string* line);
        bool get_thread_node(xml_node<>* node_file, int* thread_id, std::string* thread_name);
        bool get_event_node(xml_node<>* node_event, int* event_id, int* num_of_params, std::string* line);
        bool get_enum_name_node(xml_node<>* node_file, int* thread_id, std::string* thread_name);
        bool get_enum_value_node(xml_node<>* node_file, int* thread_id, std::string* enum_name);
        bool get_file_node(xml_node<>* node_file, int* file_id, std::string* file_name);
        bool get_root_node(xml_node<> **node);
        bool try_load_external_xml();

        bool _init_done;
        std::string _xml_full_file_path;
        xml_document<> _xml_doc;
        std::vector<char> _document_buffer;
    };
}
