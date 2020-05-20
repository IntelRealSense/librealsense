// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "types.h"
#include "core/streaming.h"

namespace librealsense
{
    class calibrated_sensor
    {
    public:
        virtual void override_intrinsics( rs2_intrinsics const & ) = 0;
        virtual void override_extrinsics( rs2_extrinsics const & ) = 0;

        virtual rs2_dsm_params get_dsm_params() const = 0;
        virtual void override_dsm_params( rs2_dsm_params const & ) = 0;

        virtual void reset_calibration() = 0;
    };
    MAP_EXTENSION(RS2_EXTENSION_CALIBRATED_SENSOR, calibrated_sensor );
}

