#pragma once
#include <stdint.h>

class TimeStampExtractor
{
public:
	TimeStampExtractor(void);
	~TimeStampExtractor(void);

	void ZeroTimeStamps();

	// TODO: Change signature to int
    uint64_t Calculate64bTimeStampFrom32b(uint32_t TimeStampIn32);

    uint64_t GetLastTimeStamp() const;

private:
    uint64_t m_Timestamp;
    uint32_t m_LastTimeStamp32;
    uint64_t m_LastTimeStamp64;
};
