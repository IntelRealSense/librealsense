#include "string-formatter.h"
#include <regex>
#include <sstream>
#include <iomanip>

using namespace std;

StringFormatter::StringFormatter(void)
{
}


StringFormatter::~StringFormatter(void)
{
}

bool StringFormatter::GenarateMessage(const string& source, int numOfParams, const uint32_t* params, string* dest)
{
	std::map<string, string> expReplaceMap;

	if (params == nullptr && numOfParams > 0) return false;

	for (int i = 0; i < numOfParams; i++)
	{
		string regularExp[2];
		string replacement[2];
		stringstream stRegularExp[2];
		stringstream stReplacement[2];

		stRegularExp[0] << "\\{\\b(" << i << ")\\}";
		regularExp[0] = stRegularExp[0].str();

		stReplacement[0] << params[i];
		replacement[0] = stReplacement[0].str();

		expReplaceMap[regularExp[0]] = replacement[0];


		stRegularExp[1] << "\\{\\b(" << i << "):x\\}";
		regularExp[1] = stRegularExp[1].str();

		stReplacement[1] << std::hex << std::setw(2) << std::setfill('0') << params[i];
		replacement[1] = stReplacement[1].str();

		expReplaceMap[regularExp[1]] = replacement[1];
	}

	return ReplaceParams(source, expReplaceMap, dest);
}

bool StringFormatter::ReplaceParams(const string& source, const std::map<string, string>& expReplaceMap, string* dest)
{
	string sourceTemp(source);

	for (auto expReplaceIt = expReplaceMap.begin(); expReplaceIt != expReplaceMap.end(); expReplaceIt++)
	{
		string destTemp;
		regex e(expReplaceIt->first);
		regex_replace(back_inserter(destTemp), sourceTemp.begin(), sourceTemp.end(), e, expReplaceIt->second);
		sourceTemp = destTemp;

	}
	*dest = sourceTemp;
	return true;
}
