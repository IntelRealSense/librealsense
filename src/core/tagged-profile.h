// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/h/rs_sensor.h>


namespace librealsense {


typedef enum profile_tag
{
    PROFILE_TAG_SUPERSET = 1,  // to be included in enable_all
    PROFILE_TAG_DEFAULT = 2,   // to be included in default pipeline start
    PROFILE_TAG_ANY = 4,       // does not include PROFILE_TAG_DEBUG
    PROFILE_TAG_DEBUG = 8,     // tag for debug formats
} profile_tag;


struct tagged_profile
{
    rs2_stream stream;
    int stream_index;
    int width, height;
    rs2_format format;
    int fps;
    int tag;
};


}  // namespace librealsense
