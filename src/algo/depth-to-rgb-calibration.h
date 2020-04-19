// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>
#include <map>
#include "../types.h"


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    struct double3 { double x, y, z; double & operator [] ( int i ) { return (&x)[i]; } };
    struct double2 { double x, y; double & operator [] ( int i ) { return (&x)[i]; } };

    enum direction :uint8_t
    {
        deg_0, //0, 1
        deg_45, //1, 1
        deg_90, //1, 0
        deg_135, //1, -1
        deg_none
    };

    struct frame_data
    {
        size_t width;
        size_t height;
    };

    struct ir_frame_data : frame_data
    {
        std::vector<uint8_t> ir_frame;
        std::vector<double> ir_edges;
    };

    struct z_frame_data : frame_data
    {
        std::vector<uint16_t> frame;
        std::vector<double> gradient_x;
        std::vector<double> gradient_y;
        std::vector<double> edges;
        std::vector<double> supressed_edges;
        size_t n_strong_edges;
        std::vector<direction> directions;
        std::vector<double> subpixels_x;
        std::vector<double> subpixels_y;
        std::vector< uint16_t> closest;
        std::vector<double> weights;
        std::vector<double> direction_deg;
        std::vector<double3> vertices;

        std::vector<unsigned char> section_map;
        bool is_edge_distributed;
        std::vector<double>sum_weights_per_section;
        double min_max_ratio;
    };

    // TODO why "yuy2"?
    struct yuy2_frame_data : frame_data
    {
        std::vector<uint8_t> yuy2_frame;
        std::vector<uint8_t> yuy2_prev_frame;
        std::vector<double> edges;
        std::vector<double> edges_IDT;
        std::vector<double> edges_IDTx;
        std::vector<double> edges_IDTy;

        std::vector<unsigned char> section_map;
        bool is_edge_distributed;
        std::vector<double>sum_weights_per_section;
        double min_max_ratio;
    };

    struct translation
    {
        double t1;
        double t2;
        double t3;
    };

    struct rotation_in_angles
    {
        double alpha;
        double beta;
        double gamma;
    };

    struct rotation
    {
        double rot[9];
    };

    struct k_matrix
    {
        double fx;
        double fy;
        double ppx;
        double ppy;
    };

    struct calib
    {
        rotation_in_angles rot_angles;
        rotation rot;
        translation trans;
        k_matrix k_mat;
        int           width;
        int           height;
        rs2_distortion model;
        double         coeffs[5];

        calib() = default;
        calib( calib const & ) = default;
        explicit calib( rs2_intrinsics const & intrinsics, rs2_extrinsics const & extrinsics );

        rs2_intrinsics get_intrinsics() const;
        rs2_extrinsics get_extrinsics() const;

        void copy_coefs( calib& obj );
        calib operator*( double step_size );
        calib operator/( double factor );
        calib operator+( const calib& c );
        calib operator-( const calib& c );
        calib operator/( const calib& c );
        double get_norma();
        double sum();
        calib normalize();
    };

    struct optimaization_params
    {
        calib curr_calib;
        calib calib_gradients;
        double cost;
        double step_size = 0;
    };

    template <class T>
    struct coeffs
    {
        std::vector<T> x_coeffs;
        std::vector<T> y_coeffs;
    };

    struct params
    {
        params();
        
        void set_depth_resolution( size_t width, size_t height );
        void set_rgb_resolution( size_t width, size_t height );

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
        double min_weighted_edge_per_section_depth = 50;
        size_t num_of_sections_for_edge_distribution_x = 2;
        size_t num_of_sections_for_edge_distribution_y = 2;
        calib normelize_mat;

        double edge_distribution_min_max_ratio = 1;
        double grad_dir_ratio = 10;
        double grad_dir_ratio_prep = 1.5;
    };

    typedef uint16_t yuy_t;
    typedef uint8_t ir_t;
    typedef uint16_t z_t;

    class optimizer
    {
    public:
        optimizer();

        void set_yuy_data(
            std::vector< yuy_t > && yuy_data,
            std::vector< yuy_t > && prev_yuy_data,
            size_t width, size_t height );
        void set_ir_data(
            std::vector< ir_t > && ir_data,
            size_t width, size_t height );
        void set_z_data(
            std::vector< z_t > && z_data,
            rs2_intrinsics const & depth_intrinsics,
            float depth_units );

        bool is_scene_valid();
        size_t optimize( calib const & original_calibration );
        bool is_valid_results();
        calib const & get_calibration() const;
        double get_cost() const;

        // for debugging/unit-testing
        z_frame_data    const & get_z_data() const   { return _z; }
        yuy2_frame_data const & get_yuy_data() const { return _yuy; }
        ir_frame_data   const & get_ir_data() const  { return _ir; }

        // impl
    private:
        void zero_invalid_edges( z_frame_data& z_data, ir_frame_data const & ir_data );
        std::vector<direction> get_direction( std::vector<double> gradient_x, std::vector<double> gradient_y );
        std::vector<uint16_t> get_closest_edges( const z_frame_data& z_data, ir_frame_data const & ir_data, size_t width, size_t height );
        std::vector<double> blur_edges( std::vector<double> const & edges, size_t image_width, size_t image_height );
        std::vector<uint8_t> get_luminance_from_yuy2( std::vector<uint16_t> yuy2_imagh );

        std::vector<uint8_t> get_logic_edges( std::vector<double> edges );
        bool is_movement_in_images( const yuy2_frame_data & yuy );
        std::vector<double> calculate_weights( z_frame_data& z_data );
        std::vector <double3> subedges2vertices( z_frame_data& z_data, const rs2_intrinsics& intrin, double depth_units );
        optimaization_params back_tracking_line_search( const z_frame_data & z_data, const yuy2_frame_data& yuy_data, optimaization_params opt_params );
        double calc_step_size( optimaization_params opt_params );
        double calc_t( optimaization_params opt_params );
        std::pair<calib, double> calc_cost_and_grad( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib& curr_calib );
        calib intrinsics_extrinsics_to_calib( rs2_intrinsics intrin, rs2_extrinsics extrin );
        double calc_cost( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<double2>& uv );
        calib calc_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<double2>& uv, const calib& curr_calib );
        translation calc_translation_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        rotation_in_angles calc_rotation_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        k_matrix calc_k_gradients( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        std::pair< std::vector<double2>, std::vector<double>> calc_rc( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib& curr_calib );
        coeffs<translation> calc_translation_coefs( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        coeffs<rotation_in_angles> calc_rotation_coefs( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        coeffs<k_matrix> calc_k_gradients_coefs( const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib & yuy_intrin_extrin, const std::vector<double>& rc, const std::vector<double2>& xy );
        k_matrix calculate_k_gradients_y_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        k_matrix calculate_k_gradients_x_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        translation calculate_translation_y_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        translation calculate_translation_x_coeff( double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_x_alpha_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_x_beta_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_x_gamma_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_y_alpha_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_y_beta_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );
        double calculate_rotation_y_gamma_coeff( rotation_in_angles rot_angles, double3 v, double rc, double2 xy, const calib & yuy_intrin_extrin );

        bool is_edge_distributed( z_frame_data & z_data, yuy2_frame_data & yuy_data );
        void section_per_pixel( frame_data const &, size_t section_w, size_t section_h, byte * section_map );
        bool is_grad_dir_balanced(z_frame_data& z_data);
        void check_edge_distribution(std::vector<double>& sum_weights_per_section, double& min_max_ratio, bool& is_edge_distributed, double distribution_min_max_ratio, double min_weighted_edge_per_section_depth);
        void sum_per_section(std::vector< double >& sum_weights_per_section, std::vector< byte > const& section_map, std::vector< double > const& weights, size_t num_of_sections);
        params _params;
        yuy2_frame_data _yuy;
        ir_frame_data _ir;
        z_frame_data _z;
        optimaization_params _params_curr;
    };

}  // librealsense::algo::depth_to_rgb_calibration
}  // librealsense::algo
}  // librealsense
