//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "max-usable-range.h"


using namespace librealsense::algo::max_range;

float max_usable_range::get_max_range(float nest) const
{
    const float thermal = 74.5f;
    const float normalized_nest = nest / 16.0f;
    const float indoor_max_range = 9.0f;

    auto temp_range = indoor_max_range;
    auto expected_max_range = 0.0f;
    auto trimmed_max_range = 0.0f;

    // Setting max range based on Nest
    if (normalized_nest > thermal)
    {
        // Analyzing reflectivity based on 85 % reflectivity data
        temp_range = 31000.0f * std::pow(normalized_nest, -2.0f) * _processing_gain;
    }

    expected_max_range = std::min(temp_range, indoor_max_range);

    if (expected_max_range == indoor_max_range)
        trimmed_max_range = indoor_max_range;
    else if (expected_max_range >= indoor_max_range - 1.5f)
        trimmed_max_range = indoor_max_range - 1.5f;
    else if (expected_max_range >= indoor_max_range - 3.0f)
        trimmed_max_range = indoor_max_range - 3.0f;
    else if (expected_max_range >= indoor_max_range - 4.5f)
        trimmed_max_range = indoor_max_range - 4.5f;
    else if (expected_max_range >= indoor_max_range - 6.0f)
        trimmed_max_range = indoor_max_range - 6.0f;
    else if (expected_max_range >= indoor_max_range - 7.5f)
        trimmed_max_range = indoor_max_range - 7.5f;

    return trimmed_max_range;
}
