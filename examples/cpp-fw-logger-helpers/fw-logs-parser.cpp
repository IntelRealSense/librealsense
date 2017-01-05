#include "fw-logs-parser.h"
#include <regex>
#include <sstream>
#include "string-formatter.h"
#include "stdint.h"

using namespace std;


FWLogsParser::FWLogsParser(std::string xmlFilePath) : m_FWlogsFormatingOptions(xmlFilePath)
{
	m_TimeStampExtractor.ZeroTimeStamps();
}


FWLogsParser::~FWLogsParser(void)
{
}

vector<string> FWLogsParser::GetFWlogLines(const FWlogsBinaryData& FWlogsDataBinary)
{
	vector<string> stringVector;
	int numOfLines = int(FWlogsDataBinary.logsBuffer.size()) / sizeof(FwLogBinary);
	auto tempPointer = reinterpret_cast<FwLogBinary const*>(FWlogsDataBinary.logsBuffer.data());

	for (int i = 0; i < numOfLines; i++)
	{
		string line;
		auto log = const_cast<char*>(reinterpret_cast<char const*>(tempPointer));
		line = GenerateLogLine(log);
		stringVector.push_back(line);
		tempPointer++;
	}
	return stringVector;
}

string FWLogsParser::GenerateLogLine(char* FWlogs)
{
	FWlogData logData;
	FillLogData(FWlogs, &logData);

	return logData.ToString();
}

void FWLogsParser::FillLogData(const char* FWlogs, FWlogData* logData)
{
	StringFormatter regExp;
	FWlogEvent logEventData;

	auto * logBinery = reinterpret_cast<const FwLogBinary*>(FWlogs);

	//parse first DWORD
    logData->magicNumber = static_cast<uint32_t>(logBinery->Dword1.bits.magicNumber);
    logData->severity = static_cast<uint32_t>(logBinery->Dword1.bits.severity);

    logData->fileId = static_cast<uint32_t>(logBinery->Dword1.bits.fileId);
    logData->groupId = static_cast<uint32_t>(logBinery->Dword1.bits.groupId);

	//parse second DWORD
    logData->eventId = static_cast<uint32_t>(logBinery->Dword2.bits.eventId);
    logData->line = static_cast<uint32_t>(logBinery->Dword2.bits.lineId);
    logData->sequence = static_cast<uint32_t>(logBinery->Dword2.bits.seqId);

	//parse third DWORD
    logData->p1 = static_cast<uint32_t>(logBinery->Dword3.P1);
    logData->p2 = static_cast<uint32_t>(logBinery->Dword3.P2);

	//parse forth DWORD
    logData->p3 = static_cast<uint32_t>(logBinery->Dword4.P3);

	auto lastTimeStamp = m_TimeStampExtractor.GetLastTimeStamp();

	//parse fifth DWORD
	logData->timeStamp = m_TimeStampExtractor.Calculate64bTimeStampFrom32b(logBinery->Dword5.timeStamp);

	if (lastTimeStamp == 0)
	{
		logData->delta = 0;
	}
	else
	{
		logData->delta = (logData->timeStamp - lastTimeStamp)*0.00001;
	}

	m_FWlogsFormatingOptions.GetEventData(logData->eventId, &logEventData);
    uint32_t params[3] = { logData->p1, logData->p2, logData->p3 };
	regExp.GenarateMessage(logEventData.Line, logEventData.NumOfParams, params, &logData->message);

	m_FWlogsFormatingOptions.GetFileName(logData->fileId, &logData->fileName);
	m_FWlogsFormatingOptions.GetThreadName(
        static_cast<uint32_t>(logBinery->Dword1.bits.threadId), &logData->threadName);
}

void FWLogsParser::ZeroTimeStamps()
{
	m_TimeStampExtractor.ZeroTimeStamps();
}
