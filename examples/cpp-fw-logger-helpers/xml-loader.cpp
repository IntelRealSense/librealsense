#include "xml-loader.h"
#include <string.h>
#include <fstream>
#include <iostream>
#include <memory>

using namespace std;



XMLLoader::XMLLoader(string lpszXML)
	: m_documentBuffer(NULL), m_InitDone(false), m_XMLname(lpszXML)
{

}


XMLLoader::~XMLLoader(void)
{
	if (m_documentBuffer)
		delete[] m_documentBuffer;
	Close();
}

bool XMLLoader::LoadXML()
{
	if (!Init())
		return false;

	return TryLoadExternalXML();
}

bool XMLLoader::Init()
{
	if (m_InitDone)
		return false;

	m_InitDone = true;
	return true;
}

void XMLLoader::Close()
{
	//CoUninitialize();
}

bool XMLLoader::GetRootNode(xml_node<> **node)
{
	if (m_InitDone)
	{
		*node = m_XMLDoc.first_node();
		return true;
	}

	return false;
}

bool XMLLoader::GetElementByTagName(std::string tagName, xml_node<> **node)
{
	// first element.
	xml_node<> *pNodeRoot = m_XMLDoc.first_node();

	for (xml_node<> *pNode = pNodeRoot->first_node(); pNode; pNode = pNode->next_sibling())
	{
        string nodeName(pNode->name(), pNode->name() + pNode->name_size());
		if (tagName.compare(nodeName) == 0)
		{
			*node = pNode;
			return true;
		}
	}
	return false;
}

bool XMLLoader::TryLoadExternalXML()
{
	try
	{
        if (m_XMLname.empty())
			return false;

        file<> xmlFile(m_XMLname.c_str());

		////LOG_INFO("Config content:\n\n" << xmlFile.data() << "\n\n\n");
		m_documentBuffer = new char[xmlFile.size() + 2];
        memcpy(m_documentBuffer, xmlFile.data(), xmlFile.size());
		m_documentBuffer[xmlFile.size()] = '\0';
		m_documentBuffer[xmlFile.size() + 1] = '\0';
		m_XMLDoc.parse<0>(m_documentBuffer);

		return true;
	}
	catch (...)
	{
		if (m_documentBuffer != nullptr)
		{
			delete[]m_documentBuffer;
			m_documentBuffer = nullptr;
		}
        throw;
	}

	return false;
}
