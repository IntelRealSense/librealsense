// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <types.h>


namespace librealsense {
namespace algo {
namespace max_range {


struct max_usable_range_inputs
{
    float nest;
    float humidity_temp;
    int gtr;
    int apd;

    max_usable_range_inputs()
        : nest( 0.0f )
        , humidity_temp( 0.0f )
        , gtr( 0 )
        , apd( 0 )
    {
    }
};

class max_usable_range
{
public:
    max_usable_range() = default;
    float get_max_range( const max_usable_range_inputs & inputs ) const;

private:

    // Algo parameters
    const float _processing_gain = 1.75f;
    const float _long_thermal = 74.5f;
    const float _short_thermal = 10.0f;  // TBD

    enum class preset_type
    {
        SHORT,
        LONG,
        CUSTOM
    };
};

}  // namespace max_range
}  // namespace algo
}  // namespace librealsense
