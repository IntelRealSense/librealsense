//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "max-usable-range.h"


using namespace librealsense::algo::max_range;

float max_usable_range::get_max_range(float nest) const
{
    const float thermal = 74.5f;
    const float normalized_nest = nest / 16.0f;
    const float indoor_max_range = 9.0f;
    const float min_range = 3.0f;

    auto temp_range = indoor_max_range;
    auto trimmed_max_range = 0.0f;

    // Setting max range based on Nest
    if (normalized_nest > thermal)
    {
        // Analyzing reflectivity based on 85 % reflectivity data
        temp_range = 31000.0f * std::pow(normalized_nest, -2.0f) * _processing_gain;
    }

    // expected_max_range should be in range 3-9 [m] at 1 [m] resolution (rounded)
    auto expected_max_range = std::round(std::min(std::max(temp_range, min_range), indoor_max_range));

    return expected_max_range;
}
