#pragma once
#include "../third_party/rapidxml/rapidxml_utils.hpp"

using namespace rapidxml;

class xml_loader
{
public:
    xml_loader(std::string xm_full_file_path);
	~xml_loader(void);
	bool load_xml();

	bool get_element_by_tag_name(std::string tagName, xml_node<> **nodes);
	bool get_root_node(xml_node<> **node);

private:
    bool init();
    void close();
	bool try_load_external_xml();


	xml_document<> _xml_doc;
	char *_document_buffer;
	bool _init_done;
    std::string _xml_name;
    int _resource_id;
};
