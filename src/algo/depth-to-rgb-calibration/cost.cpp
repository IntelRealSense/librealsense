//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "cost.h"
#include "debug.h"

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    std::vector< double > calc_cost_per_vertex(
        z_frame_data const & z_data,
        yuy2_frame_data const & yuy_data,
        std::vector< double2 > const & uv,
        std::function< void( size_t i, double d_val, double weight, double vertex_cost ) > fn
    )
    {
        auto d_vals = biliniar_interp( yuy_data.edges_IDT, yuy_data.width, yuy_data.height, uv );

        double cost = 0;
        std::vector< double > cost_per_vertex;
        cost_per_vertex.reserve( uv.size() );

        for( size_t i = 0; i < z_data.weights.size(); i++ )
        {
            double d_val = d_vals[i];  // may be std::numeric_limits<double>::max()?
            double weight = z_data.weights[i];
            double cost = d_val * weight;
            fn( i, d_val, weight, cost );
        }

        return d_vals;
    }


    double calc_cost(
        const z_frame_data & z_data,
        const yuy2_frame_data & yuy_data,
        const std::vector< double2 > & uv,
        std::vector< double > * p_interpolated_edges // = nullptr
    )
    {
        double cost = 0;
        size_t N = 0;
        auto interpolated_edges = calc_cost_per_vertex( z_data, yuy_data, uv,
            [&]( size_t i, double d_val, double weight, double vertex_cost )
            {
                if( d_val != std::numeric_limits<double>::max() )
                {
                    cost += vertex_cost;
                    ++N;
                }
            } );
        if( p_interpolated_edges )
            *p_interpolated_edges = interpolated_edges;
        return N ? cost / N : 0.;
    }


}
}
}