#include "timestamp-extractor.h"
#include <stdint.h>

	TimeStampExtractor::TimeStampExtractor(void)
	{
	}


	TimeStampExtractor::~TimeStampExtractor(void)
	{
	}

	void TimeStampExtractor::ZeroTimeStamps()
	{
		m_Timestamp = 0;
		m_LastTimeStamp32 = 0;
		m_LastTimeStamp64 = 0;
	}

    uint64_t TimeStampExtractor::Calculate64bTimeStampFrom32b(uint32_t TimeStampIn32)
	{
		//Zero the lower 32b
		m_Timestamp &= 0xFFFFFFFF00000000;

		//Copy the lower 32b from pixels to our member.
        m_Timestamp |= static_cast<uint64_t>(TimeStampIn32) & 0x00000000FFFFFFFF;

		if (TimeStampIn32 < m_LastTimeStamp32)
		{
			m_Timestamp += 0x0000000100000000;
		}

		m_LastTimeStamp32 = TimeStampIn32;
		m_LastTimeStamp64 = m_Timestamp;
		return m_Timestamp;
	}

    uint64_t TimeStampExtractor::GetLastTimeStamp() const
	{
		return m_LastTimeStamp64;
	}
