// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    class depth_to_rgb_calibration_device
    {
    public:
        virtual void register_calibration_change_callback( calibration_change_callback_ptr ) = 0;
        virtual void trigger_depth_to_rgb_calibration() = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_DEPTH_TO_RGB_CALIBRATION_DEVICE, depth_to_rgb_calibration_device);
}

