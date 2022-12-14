//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "max-usable-range.h"

// Algo parameters
static const float PROCESSING_GAIN = 1.75f;
static const float THERMAL = 74.5f;
static const float INDOOR_MAX_RANGE = 9.0f;

// Based off of code in RS5-8358
float librealsense::algo::max_usable_range::l500::max_usable_range(float noise_esimation)
{
    const float normalized_nest = noise_esimation / 16.0f;

    auto expected_max_range = INDOOR_MAX_RANGE;

    if (normalized_nest > THERMAL)
    {
        expected_max_range = 31000.0f * pow(normalized_nest, -2.0f) * PROCESSING_GAIN;
    }

    return expected_max_range;
}
