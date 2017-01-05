#pragma once
#include "../third_party/rapidxml/rapidxml_utils.hpp"

using namespace rapidxml;

class XMLLoader
{
public:
    XMLLoader(std::string lpszXML);
	~XMLLoader(void);
	bool LoadXML();

	bool GetElementByTagName(std::string tagName, xml_node<> **nodes);
	bool GetRootNode(xml_node<> **node);

	//CComPtr<IXMLDOMDocument>& operator->();
private:
	bool Init();
	void Close();
	bool TryLoadExternalXML();


	xml_document<> m_XMLDoc;
	char *m_documentBuffer;
	bool m_InitDone;
    std::string m_XMLname;
	int m_ResourceID;
};
