#pragma once
#include <string>
#include <vector>
#include "fw-logs-formating-options.h"
#include "fw-log-data.h"
#include "timestamp-extractor.h"

class FWLogsParser
{
public:
    explicit FWLogsParser(std::string xmlFilePath);
	~FWLogsParser(void);
	std::vector<std::string> GetFWlogLines(const FWlogsBinaryData& FWlogsDataBinary);

private:
	std::string GenerateLogLine(char* FWlogs);
	void FillLogData(const char* FWlogs, FWlogData* logData);
	void ZeroTimeStamps();

    FWlogsFormatingOptions m_FWlogsFormatingOptions;
	TimeStampExtractor m_TimeStampExtractor;
};
