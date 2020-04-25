// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration-types.h"
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

    coeffs< translation > calc_translation_coefs(
        const z_frame_data& z_data,
        const yuy2_frame_data& yuy_data,
        const calib & yuy_intrin_extrin,
        const std::vector< double > & rc,
        const std::vector< double2 > & xy
    );
    
    coeffs< rotation_in_angles > calc_rotation_coefs(
        const z_frame_data& z_data,
        const yuy2_frame_data& yuy_data,
        const calib & yuy_intrin_extrin,
        const std::vector<double>& rc,
        const std::vector<double2>& xy
    );
    
    coeffs< k_matrix > calc_k_gradients_coefs(
        const z_frame_data& z_data,
        const yuy2_frame_data& yuy_data,
        const calib & yuy_intrin_extrin,
        const std::vector<double>& rc,
        const std::vector<double2>& xy
    );

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
