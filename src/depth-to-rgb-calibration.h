// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "algo/depth-to-rgb-calibration.h"
#include "types.h"


namespace librealsense
{
    /*
        Wrapper around the algo version, taking actual frames and translating to algo
        vectors. Also translates the various stages in algo to actual RS2_CALIBRATION_
        return values.
    */
    class depth_to_rgb_calibration
    {
        // inputs
        stream_profile_interface* const _from;
        stream_profile_interface* const _to;

        // input/output
        rs2_extrinsics _extr;
        rs2_intrinsics _intr;

        algo::depth_to_rgb_calibration::optimizer _algo;

    public:
        depth_to_rgb_calibration(
            rs2::frame depth,
            rs2::frame ir,
            rs2::frame yuy,
            rs2::frame prev_yuy
        );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_intrinsics() const { return _intr; }
        stream_profile_interface * get_from_profile() const { return _from; }
        stream_profile_interface * get_to_profile() const { return _to; }

        rs2_calibration_status optimize();

    private:
        void debug_calibration( char const * prefix );
    };

}  // librealsense

