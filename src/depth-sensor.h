// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {
class depth_sensor
{
public:
    virtual float get_depth_scale() const = 0;
    virtual ~depth_sensor() = default;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_SENSOR, librealsense::depth_sensor );

class depth_stereo_sensor
    : public virtual depth_sensor
{
public:
    virtual float get_stereo_baseline_mm() const = 0;
};

MAP_EXTENSION( RS2_EXTENSION_DEPTH_STEREO_SENSOR, librealsense::depth_stereo_sensor );


}  // namespace librealsense
