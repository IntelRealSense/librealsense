// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "core/streaming.h"

namespace librealsense
{
    class debug_streaming_sensor
    {
    public:
        virtual stream_profiles get_debug_stream_profiles() = 0;
    };
    MAP_EXTENSION( RS2_EXTENSION_DEBUG_STREAM_SENSOR, debug_streaming_sensor );
    }
