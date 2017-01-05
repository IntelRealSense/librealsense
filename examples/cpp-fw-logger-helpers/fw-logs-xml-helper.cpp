#include "fw-logs-xml-helper.h"


using namespace std;


FWlogsXMLHelper::FWlogsXMLHelper(std::string xmlFilePath)
    : m_XmlLoader(xmlFilePath),
	m_InitDone(false)
{
}


FWlogsXMLHelper::~FWlogsXMLHelper(void)
{
	// TODO: Add cleanup code
}

bool FWlogsXMLHelper::Init()
{
	m_InitDone = GetLoader().LoadXML();
	return m_InitDone;
}

bool FWlogsXMLHelper::BuildLogMetaData(FWlogsFormatingOptions* logMetaData)
{
	xml_node<> *XMLRootNodeList;

	if (!Init())
		return false;

	if (!GetLoader().GetRootNode(&XMLRootNodeList))
	{
		return false;
	}

    string RootName(XMLRootNodeList->name(), XMLRootNodeList->name() + XMLRootNodeList->name_size());

	// check if Format is the first root name.
	if (RootName.compare("Format") != 0)
		return false;

	xml_node<> *pEventsNode = XMLRootNodeList->first_node();


	if (!BuildMetaDataStructure(pEventsNode, logMetaData))
		return false;

	return true;
}


bool FWlogsXMLHelper::BuildMetaDataStructure(xml_node<> *XMLnodeListOfEvents, FWlogsFormatingOptions* logsFormatingOptions)
{
	NodeType res = None;
	int ID;
	int NumOfParams;
	string line;

	// loop through all elements in the Format.


	for (xml_node<> *pNode = XMLnodeListOfEvents; pNode; pNode = pNode->next_sibling())
	{
		line.clear();
		res = GetNextNode(pNode, &ID, &NumOfParams, &line);
		if (res == Event)
		{
			FWlogEvent logEvent(NumOfParams, line);
			logsFormatingOptions->m_FWlogsEventList.insert(pair<int, FWlogEvent>(ID, logEvent));
		}
		else if (res == File)
		{
			logsFormatingOptions->m_FWlogsFileNamesList.insert(pair<int, string>(ID, line));
		}
		else if (res == Thread)
		{
			logsFormatingOptions->m_FWlogsThreadNamesList.insert(pair<int, string>(ID, line));
		}
	}

	return true;
}

FWlogsXMLHelper::NodeType FWlogsXMLHelper::GetNextNode(xml_node<> *node, int* ID, int* NumOfParams, string* line)
{

    string Tag(node->name(), node->name() + node->name_size());

	if (Tag.compare("Event") == 0)
	{
		if (getEventNode(node, ID, NumOfParams, line))
			return Event;
	}
	else if (Tag.compare("File") == 0)
	{
		if (GetFileNode(node, ID, line))
			return File;
	}
	else if (Tag.compare("Thread") == 0)
	{
		if (GetThreadNode(node, ID, line))
			return Thread;
	}
	return None;
}

bool FWlogsXMLHelper::GetThreadNode(xml_node<> *NodeFile, int* threadId, string* threadName)
{
	for (xml_attribute<> *pAttribute = NodeFile->first_attribute(); pAttribute; pAttribute = pAttribute->next_attribute())
	{
        string attr(pAttribute->name(), pAttribute->name() + pAttribute->name_size());

		if (attr.compare("id") == 0)
		{
            string idAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
            *threadId = stoi(idAttrStr);
			continue;
		}

		if (attr.compare("Name") == 0)
		{
            string NameAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
			*threadName = NameAttrStr;
			continue;
		}
	}

	return true;
}

bool FWlogsXMLHelper::GetFileNode(xml_node<> *NodeFile, int* fileId, string* fileName)
{
	for (xml_attribute<> *pAttribute = NodeFile->first_attribute(); pAttribute; pAttribute = pAttribute->next_attribute())
	{
        string attr(pAttribute->name(), pAttribute->name() + pAttribute->name_size());

		if (attr.compare("id") == 0)
		{
            string idAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
            *fileId = stoi(idAttrStr);
			continue;
		}

		if (attr.compare("Name") == 0)
		{
            string NameAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
			*fileName = NameAttrStr;
			continue;
		}
	}
	return true;
}

XMLLoader& FWlogsXMLHelper::GetLoader()
{
	return m_XmlLoader;
}

bool FWlogsXMLHelper::getEventNode(xml_node<> *NodeEvent, int* eventId, int* NumOfParams, string* line)
{
	for (xml_attribute<> *pAttribute = NodeEvent->first_attribute(); pAttribute; pAttribute = pAttribute->next_attribute())
	{
        string attr(pAttribute->name(), pAttribute->name() + pAttribute->name_size());

		if (attr.compare("id") == 0)
		{
            string idAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
            *eventId = stoi(idAttrStr);
			continue;
		}

		if (attr.compare("numberOfArguments") == 0)
		{
            string numOfArgsAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
             *NumOfParams = stoi(numOfArgsAttrStr);
			continue;
		}

		if (attr.compare("format") == 0)
		{
            string formatAttrStr(pAttribute->value(), pAttribute->value() + pAttribute->value_size());
			*line = formatAttrStr;
			continue;
		}
	}
	return true;

}
