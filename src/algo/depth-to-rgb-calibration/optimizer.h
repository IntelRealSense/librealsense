// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include <types.h>

#include "calibration.h"
#include "coeffs.h"
#include "frame-data.h"


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {


    struct optimaization_params
    {
        calib curr_calib;
        calib calib_gradients;
        double cost;
        double step_size = 0;
    };

    struct params
    {
        params();

        void set_depth_resolution(size_t width, size_t height);
        void set_rgb_resolution(size_t width, size_t height);

        double gamma = 0.9;
        double alpha = (double)1 / (double)3;
        double grad_ir_threshold = 3.5; // Ignore pixels with IR gradient less than this (resolution-dependent!)
        int grad_z_threshold = 25; //Ignore pixels with Z grad of less than this
        double grad_z_min = 25; // Ignore pixels with Z grad of less than this
        double grad_z_max = 1000;
        double edge_thresh4_logic_lum = 0.1;

        double max_step_size = 1;
        double min_step_size = 0.00001;
        double control_param = 0.5;
        int max_back_track_iters = 50;
        int max_optimization_iters = 50;
        double min_rgb_mat_delta = 0.00001;
        double min_cost_delta = 1;
        double tau = 0.5;
        double min_weighted_edge_per_section = 19.5313;
        size_t num_of_sections_for_edge_distribution_x = 2;
        size_t num_of_sections_for_edge_distribution_y = 2;
        calib normelize_mat;

        double edge_distribution_min_max_ratio = 0.005;
        double grad_dir_ratio = 10;
        double grad_dir_ratio_prep = 1.5;
        size_t dilation_size = 3;
        double gauss_sigma = 1;
        size_t gause_kernel_size = 5;
        double move_thresh_pix_val = 20;
        double move_threshold_pix_num = 62.2080;

        //smearing
        double max_sub_mm_z = 4;
        double constant_weights = 1000;
        double k_depth[3][3] = { {731.27344,0,529.27344 },{0,731.97656,402.32031},{0,0,1} };
        double k_depth_pinv_trans[3][3] = { { 0.0013674778, -1.1641532e-10,	2.3283064e-10 }, //pinv(params.Kdepth)';
                                            { 8.7311491e-11,0.0013661643 ,-2.9103830e-11}, 
                                            {  -0.72376943, -0.54963547,0.99999988 } };
        // output validation
        double const max_xy_movement_per_calibration[3] = { 10, 2, 2 };
        double const max_xy_movement_from_origin = 20;
    };

    // Data that's passed to a callback at each optimization iteration
    struct iteration_data_collect
    {
        size_t iteration;
        optimaization_params params;
        std::vector< double2 > uvmap;
        std::vector< double > d_vals;
        std::vector< double > d_vals_x;
        std::vector< double > d_vals_y;
        std::vector < double2> xy;
        std::vector<double > rc;
        coeffs<k_matrix> coeffs_k;
        coeffs<rotation_in_angles> coeffs_r;
        coeffs < p_matrix> coeffs_p;
    };

    class optimizer
    {
    public:
        optimizer();

        void set_yuy_data(
            std::vector< yuy_t > && yuy_data,
            std::vector< yuy_t > && prev_yuy_data,
            calib const & calibration );
        void set_ir_data(
            std::vector< ir_t > && ir_data,
            size_t width, size_t height );
        /*void set_z_data(
            std::vector< z_t > && z_data,
            rs2_intrinsics_double const & depth_intrinsics,
            float depth_units );*/
        void set_depth_data(
            std::vector< z_t >&& z_data,
            std::vector< ir_t >&& ir_data,
            rs2_intrinsics_double const& depth_intrinsics,
            float depth_units);
        // Write dumps of all the pertinent data from the above to a directory of choice, so that
        // a reproduction can be made
        void write_data_to( std::string const & directory );

        // (optional) Return whether the scene passed in is valid and can go thru optimization
        bool is_scene_valid();

        // Optimize the calibration, optionally calling the callback after each iteration
        size_t optimize( std::function< void( iteration_data_collect const & data ) > iteration_callback = nullptr );

        // (optional) Return whether the results of calibration are valid:
        //      1. If pixel movement is acceptable
        //      2. Movement from the camera's factory settings is within thresholds
        //      3. No worsening of the fit in any image section
        bool is_valid_results();

        calib const & get_calibration() const;
        double get_cost() const;
        double calc_correction_in_pixels( calib const & from_calibration ) const;
        double calc_correction_in_pixels() const { return calc_correction_in_pixels( _original_calibration ); }

        // for debugging/unit-testing
        z_frame_data    const & get_z_data() const   { return _z; }
        yuy2_frame_data const & get_yuy_data() const { return _yuy; }
        ir_frame_data   const & get_ir_data() const  { return _ir; }
        z_frame_data    const& get_depth_data() const { return _z; }
        // impl
    private:
        void zero_invalid_edges( z_frame_data& z_data, ir_frame_data const & ir_data );
        std::vector<direction> get_direction( std::vector<double> gradient_x, std::vector<double> gradient_y );
        std::vector<direction> get_direction2(std::vector<double> gradient_x, std::vector<double> gradient_y);
        std::vector<uint16_t> get_closest_edges( const z_frame_data& z_data, ir_frame_data const & ir_data, size_t width, size_t height );
        std::vector<double> blur_edges( std::vector<double> const & edges, size_t image_width, size_t image_height );
        std::vector<uint8_t> get_luminance_from_yuy2( std::vector<uint16_t> yuy2_imagh );

        std::vector<uint8_t> get_logic_edges( std::vector<double> edges );
        //std::vector<double> calculate_weights( z_frame_data& z_data );
        std::vector <double3> subedges2vertices(z_frame_data& z_data, const rs2_intrinsics_double& intrin, double depth_units);
        
        optimaization_params back_tracking_line_search( const z_frame_data & z_data, const yuy2_frame_data& yuy_data, optimaization_params opt_params );
        double calc_step_size( optimaization_params opt_params );
        double calc_t( optimaization_params opt_params );
        rotation_in_angles calc_rotation_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );

        // input validation
        bool is_movement_in_images(yuy2_frame_data& yuy);
        bool is_edge_distributed( z_frame_data & z_data, yuy2_frame_data & yuy_data );
        void section_per_pixel( frame_data const &, size_t section_w, size_t section_h, byte * section_map );
        bool is_grad_dir_balanced(z_frame_data& z_data);
        void check_edge_distribution(std::vector<double>& sum_weights_per_section, double& min_max_ratio, bool& is_edge_distributed);
        void sum_per_section(std::vector< double >& sum_weights_per_section, std::vector< byte > const& section_map, std::vector< double > const& weights, size_t num_of_sections);
        void images_dilation(yuy2_frame_data& yuy);
        void gaussian_filter(yuy2_frame_data& yuy);

        // output validation
        void clip_pixel_movement( size_t iteration_number = 0 );
        std::vector< double > cost_per_section_diff( calib const & old_calib, calib const & new_calib );

    private:
        params _params;
        yuy2_frame_data _yuy;
        ir_frame_data _ir;
        //z_frame_data _z;
        z_frame_data _z;
        calib _original_calibration;         // starting state of auto-calibration
        calib _factory_calibration;          // factory default calibration of the camera
        optimaization_params _params_curr;   // last-known setting
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
