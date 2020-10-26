// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "algo/depth-to-rgb-calibration/optimizer.h"
#include "algo/thermal-loop/thermal-calibration-table-interface.h"
#include "types.h"
#include <vector>


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
        rs2_intrinsics _raw_intr;      // raw intrinsics for overriding the fw intrinsics
        rs2_intrinsics _thermal_intr; // intrinsics with k_thermal for user
        rs2_dsm_params _dsm_params;
        std::vector< algo::depth_to_rgb_calibration::yuy_t > _last_successful_frame_data;
        algo::thermal_loop::thermal_calibration_table_interface const & _thermal_table;
        algo::depth_to_rgb_calibration::optimizer _algo;
        std::function<void()> _should_continue;

    public:
        depth_to_rgb_calibration(
            algo::depth_to_rgb_calibration::optimizer::settings const & settings,
            rs2::frame depth,
            rs2::frame ir,
            rs2::frame yuy,
            rs2::frame prev_yuy,
            std::vector< algo::depth_to_rgb_calibration::yuy_t > const & last_yuy_data,
            algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info,
            algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs,
            rs2_intrinsics const & yuy_intrinsics,
            algo::thermal_loop::thermal_calibration_table_interface const &,
            std::function<void()> should_continue = nullptr
        );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_raw_intrinsics() const { return _raw_intr; }
        rs2_intrinsics const & get_thermal_intrinsics() const { return _thermal_intr; }
        stream_profile_interface * get_from_profile() const { return _from; }
        stream_profile_interface * get_to_profile() const { return _to; }
        rs2_dsm_params const & get_dsm_params() const { return _dsm_params; }
        std::vector< algo::depth_to_rgb_calibration::yuy_t > & get_last_successful_frame_data()
        {
            return _last_successful_frame_data;
        }

        void write_data_to( std::string const & dir );

        rs2_calibration_status optimize( std::function<void( rs2_calibration_status )> call_back = nullptr);

    private:
        void debug_calibration( char const * prefix );
    };
}  // librealsense

