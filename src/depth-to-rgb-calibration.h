// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "algo/depth-to-rgb-calibration/optimizer.h"
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
        rs2_dsm_params _dsm_params;

        algo::depth_to_rgb_calibration::optimizer _algo;

    public:
        depth_to_rgb_calibration(
            rs2::frame depth,
            rs2::frame ir,
            rs2::frame yuy,
            rs2::frame prev_yuy,
            algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info,
            algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs
        );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_intrinsics() const { return _intr; }
        stream_profile_interface * get_from_profile() const { return _from; }
        stream_profile_interface * get_to_profile() const { return _to; }
        rs2_dsm_params const & get_dsm_params() const { return _dsm_params; }

        rs2_calibration_status optimize( std::function<void( rs2_calibration_status )> call_back = nullptr );

    private:
        void debug_calibration( char const * prefix );
    };
}  // librealsense

