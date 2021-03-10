// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>
#include <map>
#include <types.h>

#include "calibration-types.h"


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    struct frame_data
    {
        size_t width;
        size_t height;
    };

    typedef uint16_t yuy_t;
    typedef uint8_t ir_t;
    typedef uint16_t z_t;

    struct ir_frame_data : frame_data
    {
        std::vector< ir_t > ir_frame;
        std::vector<double> edges; // for debug
        std::vector< double > local_edges;

        // smearing
        std::vector<double> gradient_x; // for debug
        std::vector<double> gradient_y; // for debug
        std::vector< byte > valid_edge_pixels_by_ir; // for debug
        std::vector<byte> valid_section_map; // for debug
        std::vector<double>valid_gradient_x; // for debug
        std::vector<double>valid_gradient_y; // for debug
        std::vector<direction> directions; // for debug
        std::vector<double> direction_deg; // for debug
        std::vector<double> valid_location_rc_x; // for debug
        std::vector<double> valid_location_rc_y; // for debug
        std::vector<double> valid_location_rc; // for debug
        std::vector<double> direction_per_pixel; // for debug
        std::vector< double > local_region[4]; // for debug
        std::vector<double> local_region_x[4]; //for debug
        std::vector<double> local_region_y[4]; // for debug
        std::vector< byte > is_supressed;
        std::vector<double> fraq_step; // for debug

    };

    struct z_frame_data : frame_data
    {
        rs2_intrinsics_double orig_intrinsics;
        rs2_intrinsics_double new_intrinsics;
        rs2_dsm_params orig_dsm_params;
        /*algo_calibration_registers algo_calibration_registers;
        regs regs;*/
        float depth_units;

        std::vector< z_t > frame;
        std::vector<double> gradient_x; // for debug
        std::vector<double> gradient_y; // for debug
        std::vector<double> edges; // for debug
        std::vector< byte > supressed_edges;
        size_t n_strong_edges;
        std::vector<double> subpixels_x;
        std::vector<double> subpixels_y;
        std::vector<double> subpixels_y_round; // for debug
        std::vector<double> subpixels_x_round; // for debug
        std::vector<uint8_t> relevant_pixels_image;

        std::vector<double> weights;

        //smearing
        std::vector< byte > section_map_depth; // for debug
        std::vector< double > local_x; // for debug
        std::vector< double > local_y; // for debug
        std::vector< double > gradient; // for debug
        std::vector< double > local_values; // for debug
        std::vector< double > grad_in_direction; // for debug
        std::vector< double > grad_in_direction_valid; // for debug
        std::vector<double> values_for_subedges; // for debug
        std::vector<double> closest;
        std::vector<double> valid_direction_per_pixel; // for debug
        std::vector< byte > valid_section_map; // for debug
        std::vector< double > local_rc_subpixel; // for debug
        std::vector<double> edge_sub_pixel; // for debug
        std::vector<double> valid_directions; // for debug
        std::vector<double> directions;
        std::vector< double > valid_edge_sub_pixel; // for debug
        std::vector<double> sub_points; // for debug
        std::vector<double3> vertices; // for debug
        std::vector<double3> orig_vertices;
        //std::vector<double> vertices3;
        std::vector< double2 > uvmap; // for debug
        std::vector< byte > is_inside; // for debug
        // input validation
        std::vector<byte> section_map;
        bool is_edge_distributed;
        std::vector<double>sum_weights_per_section;
        std::vector<double> sum_weights_per_direction;
        double min_max_ratio;

        // output validation
        std::vector< double > cost_diff_per_section;

        //svm
        double dir_ratio1;
        k_matrix k_depth_pinv;
    };

    struct movement_inputs_for_frame
    {
        std::vector<double> const& edges;
        std::vector<uint8_t> const& lum_frame;
    };

    struct movement_result_data
    {
        std::vector<uint8_t> dilated_image;
        std::vector<uint8_t> logic_edges;
        std::vector<double> yuy_diff;
        std::vector<double> gaussian_filtered_image;
        std::vector<double> gaussian_diff_masked;
        std::vector<uint8_t> move_suspect;
    };

    struct yuy2_frame_data : frame_data
    {
        std::vector< yuy_t > orig_frame;
        std::vector< yuy_t > prev_frame;
        std::vector< yuy_t > last_successful_frame;
     
        struct
        {
            std::vector< uint8_t > lum_frame;
            std::vector< uint8_t > prev_lum_frame;
            std::vector< uint8_t > last_successful_lum_frame;

            movement_result_data movement_result;
            movement_result_data movement_prev_valid_result;

            std::vector< double > edges;  // W*H, pre-smearing
        }
        debug;

        bool movement_from_prev_frame;
        bool movement_from_last_success;

        std::vector<double> edges_IDT;                      // W*H, smeared, for cost
        std::vector<double> edges_IDTx;                     // W*H, smeared, dedge/dx, for gradients
        std::vector<double> edges_IDTy;                     // W*H, smeared, dedge/dy, for gradients
        std::vector< byte > section_map_edges;              // > params.gradRgbTh
        bool is_edge_distributed;
        std::vector<double>sum_weights_per_section;
        double min_max_ratio;
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
