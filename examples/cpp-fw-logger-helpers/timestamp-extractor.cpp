#include "timestamp-extractor.h"
#include <stdint.h>

    timestamp_extractor::timestamp_extractor(void)
	{
	}


    timestamp_extractor::~timestamp_extractor(void)
	{
	}

    void timestamp_extractor::zero_timestamps()
	{
        _timestamp = 0;
        _last_timestamp_32 = 0;
        _last_timestamp_64 = 0;
	}

    uint64_t timestamp_extractor::calculate_64b_timestamp_from_32b(uint32_t timestamp_in_32)
	{
		//Zero the lower 32b
        _timestamp &= 0xFFFFFFFF00000000;

		//Copy the lower 32b from pixels to our member.
        _timestamp |= static_cast<uint64_t>(timestamp_in_32) & 0x00000000FFFFFFFF;

        if (timestamp_in_32 < _last_timestamp_32)
		{
            _timestamp += 0x0000000100000000;
		}

        _last_timestamp_32 = timestamp_in_32;
        _last_timestamp_64 = _timestamp;
        return _timestamp;
	}

    uint64_t timestamp_extractor::get_last_timestamp() const
	{
        return _last_timestamp_64;
	}
