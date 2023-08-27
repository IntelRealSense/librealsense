// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/extension.h"

namespace librealsense {


class color_sensor
{
public:
    virtual ~color_sensor() = default;
};

MAP_EXTENSION( RS2_EXTENSION_COLOR_SENSOR, librealsense::color_sensor );


}  // namespace librealsense
