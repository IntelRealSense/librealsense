// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    // This extension should allow generic calibrations using the same interface
    // by adding to rs2_calibration_type instead of adding new function calls
    class calibration_change_notifier
    {
    public:
        virtual void register_calibration_change_callback(calibration_change_callback_ptr) = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_CALIBRATION_CHANGE_NOTIFIER, calibration_change_notifier);

    // This extension should allow generic calibrations using the same interface
    // by adding to rs2_calibration_type instead of adding new function calls
    class device_calibration : public calibration_change_notifier
    {
    public:
        virtual void trigger_device_calibration( rs2_calibration_type ) = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_DEVICE_CALIBRATION, device_calibration );
}
