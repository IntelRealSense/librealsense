#pragma once
#include <stdint.h>

class timestamp_extractor
{
public:
	timestamp_extractor(void);
	~timestamp_extractor(void);

	void zero_timestamps();

	// TODO: Change signature to int
    uint64_t calculate_64b_timestamp_from_32b(uint32_t timestamp_in_32);

    uint64_t get_last_timestamp() const;

private:
    uint64_t _timestamp;
    uint32_t _last_timestamp_32;
    uint64_t _last_timestamp_64;
};
