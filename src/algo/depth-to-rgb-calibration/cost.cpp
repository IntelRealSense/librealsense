//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "cost.h"
#include "debug.h"

namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    std::vector< double > calc_cost_per_vertex(
        std::vector<double> const & d_vals,
        z_frame_data const & z_data,
        yuy2_frame_data const & yuy_data,
        std::function< void( size_t i, double d_val, double weight, double vertex_cost ) > fn
    )
    {
        double cost = 0;
        std::vector< double > cost_per_vertex(d_vals.size());

        for (size_t i = 0; i < z_data.weights.size(); i++)
        {
            double d_val = d_vals[i];  // may be std::numeric_limits<double>::max()?
            double weight = z_data.weights[i];
            double cost = d_val;
            if (d_val != std::numeric_limits<double>::max())
                cost = d_val * weight;

            cost_per_vertex[i] = cost;
            fn(i, d_val, weight, cost);

        }
        return cost_per_vertex;
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

        auto d_vals = biliniar_interp(yuy_data.edges_IDT, yuy_data.width, yuy_data.height, uv);

        auto cost_per_vertex = calc_cost_per_vertex(d_vals, z_data, yuy_data,
            [&]( size_t i, double d_val, double weight, double vertex_cost )
            {
                if( d_val != std::numeric_limits<double>::max() )
                {
                    cost += vertex_cost;
                    ++N;
                }
            } );
        if( p_interpolated_edges )
            *p_interpolated_edges = d_vals;
        return N ? cost / N : 0.;
    }


}
}
}