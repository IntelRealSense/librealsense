#include "fw-logs-xml-helper.h"


using namespace std;


fw_logs_xml_helper::fw_logs_xml_helper(std::string xml_full_file_path)
    : _xml_loader(xml_full_file_path),
      _init_done(false)
{
}


fw_logs_xml_helper::~fw_logs_xml_helper(void)
{
	// TODO: Add cleanup code
}

bool fw_logs_xml_helper::init()
{
    _init_done = get_loader().load_xml();
    return _init_done;
}

bool fw_logs_xml_helper::build_log_meta_data(fw_logs_formating_options* log_meta_data)
{
    xml_node<> *xml_root_node_list;

    if (!init())
		return false;

    if (!get_loader().get_root_node(&xml_root_node_list))
	{
		return false;
	}

    string root_name(xml_root_node_list->name(), xml_root_node_list->name() + xml_root_node_list->name_size());

	// check if Format is the first root name.
    if (root_name.compare("Format") != 0)
		return false;

    xml_node<>* events_node = xml_root_node_list->first_node();


    if (!build_meta_data_structure(events_node, log_meta_data))
		return false;

	return true;
}


bool fw_logs_xml_helper::build_meta_data_structure(xml_node<> *xml_node_list_of_events, fw_logs_formating_options* logs_formating_options)
{
    node_type res = none;
    int id;
    int num_of_params;
	string line;

	// loop through all elements in the Format.
    for (xml_node<>* node = xml_node_list_of_events; node; node = node->next_sibling())
	{
		line.clear();
        res = get_next_node(node, &id, &num_of_params, &line);
        if (res == event)
		{
            fw_log_event log_event(num_of_params, line);
            logs_formating_options->_fw_logs_event_list.insert(pair<int, fw_log_event>(id, log_event));
		}
        else if (res == file)
		{
            logs_formating_options->_fw_logs_file_names_list.insert(pair<int, string>(id, line));
		}
        else if (res == thread)
		{
            logs_formating_options->_fw_logs_thread_names_list.insert(pair<int, string>(id, line));
		}
	}

	return true;
}

fw_logs_xml_helper::node_type fw_logs_xml_helper::get_next_node(xml_node<> *node, int* id, int* num_of_params, string* line)
{

    string tag(node->name(), node->name() + node->name_size());

    if (tag.compare("Event") == 0)
	{
        if (get_event_node(node, id, num_of_params, line))
            return event;
	}
    else if (tag.compare("File") == 0)
	{
        if (get_file_node(node, id, line))
            return file;
	}
    else if (tag.compare("Thread") == 0)
	{
        if (get_thread_node(node, id, line))
            return thread;
	}
    return none;
}

bool fw_logs_xml_helper::get_thread_node(xml_node<>* node_file, int* thread_id, string* thread_name)
{
    for (xml_attribute<>* attribute = node_file->first_attribute(); attribute; attribute = attribute->next_attribute())
	{
        string attr(attribute->name(), attribute->name() + attribute->name_size());

		if (attr.compare("id") == 0)
		{
            string id_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *thread_id = stoi(id_attr_str);
			continue;
		}

		if (attr.compare("Name") == 0)
		{
            string name_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *thread_name = name_attr_str;
			continue;
		}
	}

	return true;
}

bool fw_logs_xml_helper::get_file_node(xml_node<>* node_file, int* file_id, string* file_name)
{
    for (xml_attribute<>* attribute = node_file->first_attribute(); attribute; attribute = attribute->next_attribute())
	{
        string attr(attribute->name(), attribute->name() + attribute->name_size());

		if (attr.compare("id") == 0)
		{
            string id_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *file_id = stoi(id_attr_str);
			continue;
		}

		if (attr.compare("Name") == 0)
		{
            string name_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *file_name = name_attr_str;
			continue;
		}
	}
	return true;
}

xml_loader& fw_logs_xml_helper::get_loader()
{
    return _xml_loader;
}

bool fw_logs_xml_helper::get_event_node(xml_node<>* node_event, int* event_id, int* num_of_params, string* line)
{
    for (xml_attribute<>* attribute = node_event->first_attribute(); attribute; attribute = attribute->next_attribute())
	{
        string attr(attribute->name(), attribute->name() + attribute->name_size());

		if (attr.compare("id") == 0)
		{
            string id_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *event_id = stoi(id_attr_str);
			continue;
		}

		if (attr.compare("numberOfArguments") == 0)
		{
            string num_of_args_attr_str(attribute->value(), attribute->value() + attribute->value_size());
             *num_of_params = stoi(num_of_args_attr_str);
			continue;
		}

		if (attr.compare("format") == 0)
		{
            string format_attr_str(attribute->value(), attribute->value() + attribute->value_size());
            *line = format_attr_str;
			continue;
		}
	}
	return true;
}
