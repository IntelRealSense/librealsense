// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"

namespace librealsense
{
    class max_usable_range_sensor
    {
    public:
        virtual float get_max_usable_depth_range( ) const = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_MAX_USABLE_RANGE_SENSOR, max_usable_range_sensor);
}
