// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <types.h>


namespace librealsense {
namespace algo {
namespace max_range {

class max_usable_range
{
public:
    max_usable_range() = default;
    // Calculate the maximum usable range based on current ambient light conditions
    // Input: noise estimation value
    // Output: maximum usable range [m] 
    float get_max_range( float nest ) const;

private:

    // Algo parameters
    const float _processing_gain = 1.75f;
};

}  // namespace max_range
}  // namespace algo
}  // namespace librealsense
