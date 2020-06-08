// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "../algo-common.h"
#include "../../../src/algo/depth-to-rgb-calibration/optimizer.h"
#include "scene-data.h"
#include "../../../src/algo/depth-to-rgb-calibration/k-to-dsm.h"
#include "../../../src/algo/depth-to-rgb-calibration/utils.h"
#include "../../../src/algo/depth-to-rgb-calibration/uvmap.h"


#include "ac-logger.h"
ac_logger LOG_TO_STDOUT;


namespace algo = librealsense::algo::depth_to_rgb_calibration;
using librealsense::to_string;



void init_algo( algo::optimizer & cal,
    std::string const & dir,
    std::string const & yuy,
    std::string const & yuy_prev,
    std::string const & ir,
    std::string const & z,
    camera_params const & camera
)
{
    algo::calib calibration( camera.rgb, camera.extrinsics );

    cal.set_yuy_data(
        read_image_file< algo::yuy_t >( dir + yuy, camera.rgb.width, camera.rgb.height ),
        read_image_file< algo::yuy_t >( dir + yuy_prev, camera.rgb.width, camera.rgb.height ),
        calibration
    );

    cal.set_ir_data(
        read_image_file< algo::ir_t >( dir + ir, camera.z.width, camera.z.height ),
        camera.z.width, camera.z.height
    );

    cal.set_z_data(
        read_image_file< algo::z_t >( dir + z, camera.z.width, camera.z.height ),
        camera.z, camera.dsm_params, camera.cal_info, camera.cal_regs, camera.z_units
    );
}
