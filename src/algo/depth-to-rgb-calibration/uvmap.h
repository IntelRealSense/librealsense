// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration.h"


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    /*
    A UV-map is simply a collection of 2D points on the RGB frame.

    In our case, the point may not fall neatly on (integer) coordinates but,
    rather, in-between! In this case, bilinear interpolation can be used to
    actually transform any another WxH (per-coordinate) value matrix into a
    per-UV-pixel vector of interpolated values.
    */
    typedef std::vector< double2 > uvmap_t;


    // Map 3D points to a UV-map given a calibration
    uvmap_t get_texture_map(
        std::vector< double3 > const & vertices,
        calib const & calibration,
        p_matrix const & p_mat
    );


    // Interpolate WxH values (unit unspecified) according to a (N-sized)
    // vector of (x,y) pixels to an (N-sized) output vector of values
    std::vector< double > biliniar_interp(
        std::vector< double > const & values,
        size_t width,
        size_t height,
        uvmap_t const & uvmap
    );


}
}
}
