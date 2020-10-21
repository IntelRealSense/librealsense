//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "max-usable-range.h"


using namespace librealsense::algo::max_range;

float max_usable_range::get_max_range(const max_usable_range_inputs &inputs) const
{

    auto thermal = 0.0f;
    auto normalized_nest = 0.0f;
    auto indoor_max_range = 9.0f;

    auto m_factor = 170.0f / (std::pow(1.1f, inputs.apd - 9.0f));

    preset_type preset = preset_type::LONG;

    if ((inputs.apd == 18) && (inputs.gtr == 3))
        preset = preset_type::SHORT;
    else if ((inputs.apd == 9) && (inputs.gtr == 0))
        preset = preset_type::LONG;
    else
        preset = preset_type::CUSTOM;

    if (preset == preset_type::LONG)
    {
        thermal = _long_thermal;
        normalized_nest = inputs.nest / 16.0f;
        indoor_max_range = 9.0f;
    }
    else if (preset == preset_type::SHORT)
    {
        thermal = _short_thermal;
        indoor_max_range = 6.1f;
        normalized_nest = (inputs.nest - 797.0f) / 6.275f;
    }
    else
    {
        indoor_max_range = 9.0f / std::sqrt(170.0f / m_factor);
        if (inputs.gtr == 3)
        {
            thermal = thermal = _short_thermal;
            normalized_nest = (inputs.nest - 797.0f) / 6.275f;
        }
        else
        {
            thermal = _long_thermal;
            normalized_nest = inputs.nest / 16.0f;
        }
    }

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
