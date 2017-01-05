#pragma once
#include <unordered_map>
#include <string>
#include <stdint.h>

struct FWlogEvent
{
	int NumOfParams;
	std::string Line;

	FWlogEvent();
	FWlogEvent(int inputNumOfParams, const std::string& inputLine);
};

class FWlogsXMLHelper;

class FWlogsFormatingOptions
{
public:
    FWlogsFormatingOptions(std::string xmlFilePath);
	virtual ~FWlogsFormatingOptions(void);


	virtual bool GetEventData(int id, FWlogEvent* logEventData) const;
	virtual bool GetFileName(int id, std::string* fileName) const;
    virtual bool GetThreadName(uint32_t thread_id, std::string* thread_name);

private:
    bool InitializeFromXml(std::string xmlFilePath);
	friend FWlogsXMLHelper;
	std::unordered_map<int, FWlogEvent> m_FWlogsEventList;
	std::unordered_map<int, std::string> m_FWlogsFileNamesList;
	std::unordered_map<int, std::string> m_FWlogsThreadNamesList;
};
