#pragma once
#include "fw-logs-formating-options.h"
#include "xml-loader.h"

class FWlogsXMLHelper
{
public:
	enum NodeType
	{
		Event,
		File,
		Thread,
		None
	};

    FWlogsXMLHelper(std::string xmlFilePath);
	~FWlogsXMLHelper(void);

    bool BuildLogMetaData(FWlogsFormatingOptions* logsFormatingOptions);

private:
	bool Init();
    bool BuildMetaDataStructure(xml_node<> *XMLnodeListOfEvents, FWlogsFormatingOptions* logsFormatingOptions);

	NodeType GetNextNode(xml_node<> *XMLnodeListOfEvents, int* ID, int* NumOfParams, std::string* line);
	bool GetThreadNode(xml_node<> *NodeFile, int* threadId, std::string* threadName);
	bool getEventNode(xml_node<> * NodeEvent, int* eventId, int* NumOfParams, std::string* line);
	bool GetFileNode(xml_node<> *NodeFile, int* fileId, std::string* fileName);

	XMLLoader& GetLoader();

	XMLLoader m_XmlLoader;

	bool m_InitDone;
};
