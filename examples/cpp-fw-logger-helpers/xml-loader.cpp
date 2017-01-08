#include "xml-loader.h"
#include <string.h>
#include <fstream>
#include <iostream>
#include <memory>

using namespace std;



xml_loader::xml_loader(string xm_full_file_path)
    : _document_buffer(NULL), _init_done(false), _xml_name(xm_full_file_path)
{

}


xml_loader::~xml_loader(void)
{
    if (_document_buffer)
        delete[] _document_buffer;
    close();
}

bool xml_loader::load_xml()
{
    if (!init())
		return false;

    return try_load_external_xml();
}

bool xml_loader::init()
{
	if (_init_done)
		return false;

	_init_done = true;
	return true;
}

void xml_loader::close()
{
	//CoUninitialize();
}

bool xml_loader::get_root_node(xml_node<> **node)
{
	if (_init_done)
	{
        *node = _xml_doc.first_node();
		return true;
	}

	return false;
}

bool xml_loader::get_element_by_tag_name(std::string tag_name, xml_node<>** node_xml)
{
	// first element.
    xml_node<>* node_root = _xml_doc.first_node();

    for (xml_node<>* node = node_root->first_node(); node; node = node->next_sibling())
	{
        string node_name(node->name(), node->name() + node->name_size());
        if (tag_name.compare(node_name) == 0)
		{
            *node_xml = node;
			return true;
		}
	}
	return false;
}

bool xml_loader::try_load_external_xml()
{
	try
	{
        if (_xml_name.empty())
			return false;

        file<> xml_file(_xml_name.c_str());

        _document_buffer = new char[xml_file.size() + 2];
        memcpy(_document_buffer, xml_file.data(), xml_file.size());
        _document_buffer[xml_file.size()] = '\0';
        _document_buffer[xml_file.size() + 1] = '\0';
        _xml_doc.parse<0>(_document_buffer);

		return true;
	}
	catch (...)
	{
        if (_document_buffer != nullptr)
		{
            delete[]_document_buffer;
            _document_buffer = nullptr;
		}
        throw;
	}

	return false;
}
