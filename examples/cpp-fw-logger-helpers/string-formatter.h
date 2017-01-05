#pragma once
#include <string>
#include <map>
#include <stdint.h>

class StringFormatter
{
public:
	StringFormatter(void);
	~StringFormatter(void);

    bool GenarateMessage(const std::string& source, int numOfParams, const uint32_t* params, std::string* dest);

private:
	bool ReplaceParams(const std::string& source, const std::map<std::string, std::string>& expReplaceMap, std::string* dest);
};
