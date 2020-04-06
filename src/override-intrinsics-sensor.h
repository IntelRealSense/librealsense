// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    class override_intrinsics_sensor
    {
    public:
        virtual void override_intrinsics( stream_profile_interface const *, rs2_intrinsics const & ) = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_OVERRIDE_INTRINSICS_SENSOR, override_intrinsics_sensor );
}

