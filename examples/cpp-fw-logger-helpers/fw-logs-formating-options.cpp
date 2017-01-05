#include "fw-logs-formating-options.h"
#include "fw-logs-xml-helper.h"
#include <sstream>


using namespace std;


FWlogEvent::FWlogEvent()
	:NumOfParams(0),
    Line("") {}

FWlogEvent::FWlogEvent(int inputNumOfParams, const std::string& inputLine)
	:NumOfParams(inputNumOfParams),
    Line(inputLine) {}


FWlogsFormatingOptions::FWlogsFormatingOptions(std::string xmlFilePath)
{
    InitializeFromXml(xmlFilePath);
}


FWlogsFormatingOptions::~FWlogsFormatingOptions(void)
{
}

bool FWlogsFormatingOptions::GetEventData(int id, FWlogEvent* logEventData) const
{

	auto eventIt = m_FWlogsEventList.find(id);
	if (eventIt != m_FWlogsEventList.end())
	{
		*logEventData = eventIt->second;
		return true;
	}
	else
	{
		stringstream s;
		s << "*** Unrecognized Log Id: ";
		s << id;
		s << "! P1 = 0x{0:x}, P2 = 0x{1:x}, P3 = 0x{2:x}";
		logEventData->Line = s.str();
		logEventData->NumOfParams = 3;
		return false;
	}
}

bool FWlogsFormatingOptions::GetFileName(int id, string* fileName) const
{
	auto fileIt = m_FWlogsFileNamesList.find(id);
	if (fileIt != m_FWlogsFileNamesList.end())
	{
		*fileName = fileIt->second;
		return true;
	}
	else
	{
		*fileName = "Unknown";
		return false;
	}
}

bool FWlogsFormatingOptions::GetThreadName(uint32_t thread_id, std::string* thread_name)
{
	auto fileIt = m_FWlogsThreadNamesList.find(thread_id);
	if (fileIt != m_FWlogsThreadNamesList.end())
	{
		*thread_name = fileIt->second;
		return true;
	}
	else
	{
		*thread_name = "Unknown";
		return false;
	}
}

bool FWlogsFormatingOptions::InitializeFromXml(std::string xmlFilePath)
{
    FWlogsXMLHelper FWlogsXML(xmlFilePath);
	return FWlogsXML.BuildLogMetaData(this);
}
