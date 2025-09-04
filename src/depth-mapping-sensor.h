// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 RealSense, Inc. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
    class depth_mapping_sensor
    {
    public:
        virtual ~depth_mapping_sensor() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_DEPTH_MAPPING_SENSOR, librealsense::depth_mapping_sensor);
}
