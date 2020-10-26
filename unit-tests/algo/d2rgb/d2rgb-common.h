// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "../algo-common.h"
#include "../../../src/algo/depth-to-rgb-calibration/optimizer.h"
#include "scene-data.h"
#include "../../../src/algo/depth-to-rgb-calibration/k-to-dsm.h"
#include "../../../src/algo/depth-to-rgb-calibration/utils.h"
#include "../../../src/algo/depth-to-rgb-calibration/uvmap.h"

#include "../../../src/algo/thermal-loop/l500-thermal-loop.h"

#include "ac-logger.h"
#if ! defined( DISABLE_LOG_TO_STDOUT )
ac_logger LOG_TO_STDOUT;
#endif


#include "../../profiler.h"


namespace algo = librealsense::algo::depth_to_rgb_calibration;
namespace thermal = librealsense::algo::thermal_loop;

using librealsense::to_string;



void init_algo( algo::optimizer & cal,
    std::string const & dir,
    std::string const & yuy,
    std::string const & yuy_prev,
    std::string const & yuy_last_successful,
    std::string const & ir,
    std::string const & z,
    camera_params const & camera,
    memory_profiler * profiler = nullptr
)
{
    algo::calib calibration( camera.rgb, camera.extrinsics );

    std::vector< algo::yuy_t> yuy_last_successful_frame;

    try
    {
        yuy_last_successful_frame
            = read_image_file< algo::yuy_t >( join( dir, yuy_last_successful ),
                                              camera.rgb.width,
                                              camera.rgb.height );
    }
    catch (...) 
    {
        yuy_last_successful_frame.clear();
    };

    if( profiler )
        profiler->section( "Preprocessing YUY" );
    cal.set_yuy_data(
        read_image_file< algo::yuy_t >( join( dir, yuy ), camera.rgb.width, camera.rgb.height ),
        read_image_file< algo::yuy_t >( join( dir, yuy_prev ), camera.rgb.width, camera.rgb.height ),
        std::move(yuy_last_successful_frame),
        calibration
    );
    if( profiler )
        profiler->section_end();

    if( profiler )
        profiler->section( "Preprocessing IR" );
    cal.set_ir_data(
        read_image_file< algo::ir_t >( join( dir, ir ), camera.z.width, camera.z.height ),
        camera.z.width, camera.z.height
    );
    if( profiler )
        profiler->section_end();

    if( profiler )
        profiler->section( "Preprocessing DEPTH" );
    cal.set_z_data(
        read_image_file< algo::z_t >( join( dir, z ), camera.z.width, camera.z.height ),
        camera.z, camera.dsm_params, camera.cal_info, camera.cal_regs, camera.z_units
    );
    if( profiler )
        profiler->section_end();
}
