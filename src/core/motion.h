// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once


#include "streaming.h"

namespace librealsense
{
    class motion_stream_profile_interface : public virtual stream_profile_interface
    {
    public:
        virtual rs2_motion_device_intrinsic get_intrinsics() const = 0;
        virtual void set_intrinsics(std::function<rs2_motion_device_intrinsic()> calc) = 0;
    };

    MAP_EXTENSION(RS2_EXTENSION_MOTION_PROFILE, librealsense::motion_stream_profile_interface);

    class pose_stream_profile_interface : public virtual stream_profile_interface
    {
        //Empty for now
    };

    MAP_EXTENSION(RS2_EXTENSION_POSE_PROFILE, librealsense::pose_stream_profile_interface);

    
    class loopback_interface
    {
    public:
        //Empty for now
        virtual void enable_loopback(const std::string& input) = 0;
        virtual void disable_loopback() = 0;
        virtual bool is_enabled() const = 0;
        virtual ~loopback_interface() = default;
    };
    MAP_EXTENSION(RS2_EXTENSION_LOOPBACK, librealsense::loopback_interface);
}
