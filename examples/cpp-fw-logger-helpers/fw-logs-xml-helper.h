#pragma once
#include "fw-logs-formating-options.h"
#include "xml-loader.h"

class fw_logs_xml_helper
{
public:
    enum node_type
	{
        event,
        file,
        thread,
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
    bool get_file_node(xml_node<>* node_file, int* file_id, std::string* file_name);
    xml_loader& get_loader();

    xml_loader _xml_loader;
	bool _init_done;
};
