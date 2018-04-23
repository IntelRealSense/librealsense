// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved
// The routies used to perform the actual holes filling when the depth is presented as disparity (floating point) data
#pragma once

namespace librealsense
{
    enum holes_filling_types : uint8_t
    {
        hf_disabled,
        hf_2_pixel_radius,
        hf_4_pixel_radius,
        hf_8_pixel_radius,
        hf_16_pixel_radius,
        hf_unlimited_radius,
        hf_fill_from_left,
        hf_farest_from_around,
        hf_nearest_from_around,
        hf_max_value
    };

    template <holes_filling_types T>
    void holes_filling_pass(void * image_data)
    {
    }

    template<>
    void holes_filling_pass<hf_fill_from_left>(void * image_data)
    {
    }

    template<>
    void holes_filling_pass<hf_farest_from_around>(void * image_data)
    {
    }
}
