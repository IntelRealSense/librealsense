// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration-types.h"
#include "calibration.h"
#include <vector>


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    template< class T >
    struct coeffs
    {
        std::vector<T> x_coeffs;
        std::vector<T> y_coeffs;
    };

    struct z_frame_data;
    struct yuy2_frame_data;
    struct calib;

    coeffs< p_matrix > calc_p_coefs(
        const z_frame_data& z_data,
        const std::vector<double3>& new_vertices,
        const yuy2_frame_data& yuy_data,
        const calib & cal,
        const p_matrix & p_mat,
        const std::vector<double>& rc,
        const std::vector<double2>& xy
    );

    struct data_collect;

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
