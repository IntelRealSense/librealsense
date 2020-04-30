// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "uvmap.h"
#include "frame-data.h"


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    std::vector< double > calc_cost_per_vertex(
        z_frame_data const & z_data,
        yuy2_frame_data const & yuy_data,
        uvmap_t const & uv,
        std::function< void( size_t i, double d_val, double weight, double vertex_cost ) > fn
    );


    double calc_cost(
        const z_frame_data & z_data,
        const yuy2_frame_data & yuy_data,
        const uvmap_t & uvmap,
        std::vector< double > * p_interpolated_edges = nullptr
    );


}
}
}
