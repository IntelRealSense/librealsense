// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once


#include "streaming.h"

namespace librealsense
{
    class motion_sensor_interface
    {
    public:
        virtual rs2_motion_device_intrinsic get_motion_intrinsics(rs2_stream stream) const = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION, librealsense::motion_sensor_interface);


    class motion_stream_profile_interface : public virtual stream_profile_interface
    {
    public:
        virtual rs2_motion_device_intrinsic get_intrinsics() const = 0;
        virtual void set_intrinsics(std::function<rs2_motion_device_intrinsic()> calc) = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION_PROFILE, librealsense::motion_stream_profile_interface);
}
