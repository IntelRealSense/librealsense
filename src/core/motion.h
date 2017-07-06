// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "../include/librealsense/rs2.h"

#include "streaming.h"

namespace librealsense
{
    class motion_sensor_interface : public virtual sensor_interface
    {
    public:
        virtual rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) const = 0;
    };
}
