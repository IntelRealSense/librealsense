// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "stream.h"

namespace librealsense
{
    struct stream_profile;

    class video_sensor_interface : public virtual sensor_interface
    {
    public:
        virtual rs2_intrinsics get_intrinsics(const stream_profile& profile) const = 0;
    };

    class video_stream_profile : public virtual stream_profile_base
    {
    public:
        virtual rs2_intrinsics get_intrinsics() const = 0;

        uint32_t get_width();
        uint32_t get_height();

    private:
        uint32_t _height;
        uint32_t _width;
    };

    MAP_EXTENSION(RS2_EXTENSION_TYPE_VIDEO, librealsense::video_sensor_interface);
}
