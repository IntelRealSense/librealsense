// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "core/streaming.h"

namespace librealsense
{
    struct stream_profile;

    class video_sensor_interface
    {
    public:
        virtual rs2_intrinsics get_intrinsics(const stream_profile& profile) const = 0;
    };

    class video_stream_profile_interface : public virtual stream_profile_interface
    {
    public:
        virtual rs2_intrinsics get_intrinsics() const = 0;
        virtual void set_intrinsics(std::function<rs2_intrinsics()> calc) = 0;

        virtual uint32_t get_width() const = 0;
        virtual uint32_t get_height() const = 0;
        virtual void set_dims(uint32_t width, uint32_t height) = 0;

    };

    MAP_EXTENSION(RS2_EXTENSION_VIDEO, librealsense::video_sensor_interface);
    MAP_EXTENSION(RS2_EXTENSION_VIDEO_PROFILE, librealsense::video_stream_profile_interface);
}
