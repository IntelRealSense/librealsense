// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
#pragma once

#include "streaming.h"

namespace librealsense
{
    struct stream_profile;

    class video_sensor_interface : public virtual sensor_interface
    {
    public:
        virtual rs2_intrinsics get_intrinsics(const stream_profile& profile) const = 0;
    };

    DEFINE_MAPPING(RS2_EXTENSION_TYPE_VIDEO, librealsense::video_sensor_interface);
}
