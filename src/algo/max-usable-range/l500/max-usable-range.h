// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <types.h>


namespace librealsense {
namespace algo {
namespace max_usable_range {
namespace l500 {

// Calculate the maximum usable range based on current ambient light conditions
// Input: noise estimation value
// Output: maximum usable range [m]
float max_usable_range( float nest );

}  // namespace l500
}  // namespace max_range
}  // namespace algo
}  // namespace librealsense
