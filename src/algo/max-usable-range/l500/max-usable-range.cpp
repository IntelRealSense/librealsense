//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "max-usable-range.h"

// Algo parameters
const float PROCESSING_GAIN = 1.75f;
const float THERMAL = 74.5f;
const float INDOOR_MAX_RANGE = 9.0f;
const float MIN_RANGE = 3.0f;

// Based off of code in RS5-8358
float librealsense::algo::max_usable_range::l500::max_usable_range(float nest)
{
    const float normalized_nest = nest / 16.0f;

    auto temp_range = INDOOR_MAX_RANGE;

    if (normalized_nest > THERMAL)
    {
        temp_range = 31000.0f * pow(normalized_nest, -2.0f) * PROCESSING_GAIN;
    }

    // expected_max_range should be in range 3-9 [m] at 1 [m] resolution (rounded)
    auto expected_max_range = round(std::min(std::max(temp_range, MIN_RANGE), INDOOR_MAX_RANGE));

    return expected_max_range;
}
