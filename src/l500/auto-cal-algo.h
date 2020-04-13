// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "serializable-interface.h"
#include "hw-monitor.h"
#include "sensor.h"

namespace librealsense
{
    class auto_cal_algo
    {
    public:

        enum direction :uint8_t
        {
            deg_0, //0, 1
            deg_45, //1, 1
            deg_90, //1, 0
            deg_135, //1, -1
            deg_none
        };

        struct ir_frame_data
        {
            std::vector<uint8_t> ir_frame;
            std::vector<double> ir_edges;

            int width;
            int height;
        };

        struct z_frame_data
        {
            std::vector<uint16_t> frame;
            std::vector<double> gradient_x;
            std::vector<double> gradient_y;
            std::vector<double> edges;
            std::vector<double> supressed_edges;
            std::vector<direction> directions;
            std::vector<double> subpixels_x;
            std::vector<double> subpixels_y;
            std::vector< uint16_t> closest;
            std::vector<double> weights;
            std::vector<double> direction_deg;
            std::vector<rs2_vertex> vertices;

            int width;
            int height;
        };

        struct yuy2_frame_data
        {
            std::vector<uint8_t> yuy2_frame;
            std::vector<uint8_t> yuy2_prev_frame;
            std::vector<double> edges;
            std::vector<double> edges_IDT;
            std::vector<double> edges_IDTx;
            std::vector<double> edges_IDTy;

            int width;
            int height;
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
            float rot[9];
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

            void copy_coefs(calib& obj);
            calib operator*(double step_size);
            calib operator/(double factor);
            calib operator+(const calib& c);
            calib operator-(const calib& c);
            calib operator/(const calib& c);
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

            double gamma = 0.98;
            double alpha = 0.333333333333333;
            double grad_ir_threshold = 3.5f; // Ignore pixels with IR grad of less than this
            int grad_z_threshold = 25; //Ignore pixels with Z grad of less than this
            double grad_z_min = 25.f; // Ignore pixels with Z grad of less than this
            double grad_z_max = 1000.f;
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
            double num_of_sections_for_edge_distribution_x = 2;
            double num_of_sections_for_edge_distribution_y = 2;
            calib normelize_mat;
        };

        struct std::map < direction, std::pair<int, int>> dir_map = { {deg_0, {1, 0}},
                                                                 {deg_45, {1, 1}},
                                                                 {deg_90, {0, 1}},
                                                                 {deg_135, {-1, 1}} };

        auto_cal_algo(
            rs2::frame depth,
            rs2::frame ir,
            rs2::frame yuy,
            rs2::frame prev_yuy
        );

        rs2_extrinsics const & get_extrinsics() const { return _extr; }
        rs2_intrinsics const & get_intrinsics() const { return _intr; }
        stream_profile_interface * get_from_profile() const { return _from; }
        stream_profile_interface * get_to_profile() const { return _to; }

		bool is_scene_valid() { return true; }
		static bool is_valid_params( optimaization_params const & original, optimaization_params const & now );
        rs2_calibration_status optimize();


    private:
        z_frame_data preproccess_z(rs2::frame depth, const ir_frame_data & ir_data, const rs2_intrinsics& depth_intrinsics, float depth_units);
        yuy2_frame_data preprocess_yuy2_data(rs2::frame color, rs2::frame prev_color);
        ir_frame_data preprocess_ir(rs2::frame ir);

        void zero_invalid_edges(z_frame_data& z_data, ir_frame_data ir_data);
        std::vector<double> get_direction_deg(std::vector<double> gradient_x, std::vector<double> gradient_y);
        std::vector<direction> get_direction(std::vector<double> gradient_x, std::vector<double> gradient_y);
        std::pair<uint32_t, uint32_t> get_prev_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);
        std::pair<uint32_t, uint32_t> get_next_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);
        std::vector<double> supressed_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        std::vector<uint16_t> get_closest_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        std::pair<std::vector<double>, std::vector<double>> calc_subpixels(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        std::vector<double> blure_edges(std::vector<double> edges, uint32_t image_widht, uint32_t image_height);
        std::vector<uint8_t> get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh);
        
        std::vector<uint8_t> get_logic_edges(std::vector<double> edges);
        bool is_movement_in_images(const yuy2_frame_data & yuy);
        bool is_scene_valid(yuy2_frame_data yuy);
        std::vector<double> calculate_weights(z_frame_data& z_data);
        std::vector <rs2_vertex> subedges2vertices(z_frame_data& z_data, const rs2_intrinsics& intrin, double depth_units);
        calib get_calib_gradients(const z_frame_data& z_data);
        optimaization_params back_tracking_line_search(const z_frame_data & z_data, const yuy2_frame_data& yuy_data, optimaization_params opt_params);
        double calc_step_size(optimaization_params opt_params);
        double calc_t(optimaization_params opt_params);
        std::pair<auto_cal_algo::calib, double> calc_cost_and_grad(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib& curr_calib);
        std::pair<rs2_intrinsics, rs2_extrinsics> convert_calib_to_intrinsics_extrinsics(auto_cal_algo::calib curr_calib);
        auto_cal_algo::calib intrinsics_extrinsics_to_calib(rs2_intrinsics intrin, rs2_extrinsics extrin);
        double calc_cost(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv);
        calib calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv, const calib& curr_calib);
        translation calc_translation_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        rotation_in_angles calc_rotation_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        k_matrix calc_k_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        std::pair< std::vector<float2>, std::vector<double>> calc_rc(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const calib& curr_calib);
        coeffs<translation> calc_translation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        coeffs<rotation_in_angles> calc_rotation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        coeffs<k_matrix> calc_k_gradients_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy);
        k_matrix calculate_k_gradients_y_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        k_matrix calculate_k_gradients_x_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        translation calculate_translation_y_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        translation calculate_translation_x_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_x_alpha_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_x_beta_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_x_gamma_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_y_alpha_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_y_beta_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        double calculate_rotation_y_gamma_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        void deproject_sub_pixel(std::vector<rs2_vertex>& points, const rs2_intrinsics & intrin, const double * x, const double * y, const uint16_t * depth, double depth_units);

		void debug_calibration(char const * prefix);

        params _params;

        std::vector<double> calc_intensity(std::vector<double> image1, std::vector<double> image2)
        {
            std::vector<double> res(image1.size(), 0);

            for (auto i = 0; i < image1.size(); i++)
            {
                res[i] = sqrt(pow(image1[i], 2) + pow(image2[i], 2));
            }
            return res;
        }
        template<class T>
        double dot_product(std::vector<T> sub_image, std::vector<double> mask)
        {
            double res = 0;

            for (auto i = 0; i < sub_image.size(); i++)
            {
                res += sub_image[i] * mask[i];
            }

            return res;
        }

        template<class T>
        std::vector<double> convolution(std::vector<T> image, uint32_t image_widht, uint32_t image_height, uint32_t mask_widht, uint32_t mask_height, std::function<double(std::vector<T> sub_image)> convolution_operation)
        {
            std::vector<double> res(image.size(), 0);

            for (auto i = 0; i < image_height - mask_height + 1; i++)
            {
                for (auto j = 0; j < image_widht - mask_widht + 1; j++)
                {
                    std::vector<T> sub_image(mask_widht*mask_height, 0);

                    auto ind = 0;
                    for (auto l = 0; l < mask_height; l++)
                    {
                        for (auto k = 0; k < mask_widht; k++)
                        {
                            auto p = (i + l)* image_widht + j + k;
                            sub_image[ind++] = (image[p]);
                        }

                    }
                    auto mid = (i + mask_height / 2) * image_widht + j + mask_widht / 2;
                    res[mid] = convolution_operation(sub_image);
                }
            }
            return res;
        }

        template<class T>
        std::vector<double> calc_horizontal_gradient(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            std::vector<double> horizontal_gradients = { -1, -2, -1,
                                                          0,  0,  0,
                                                          1,  2,  1 };

            return convolution<T>(image, image_widht, image_height, 3, 3, [&](std::vector<T> sub_image)
            {return dot_product(sub_image, horizontal_gradients) / (double)8; });
        }

        template<class T>
        std::vector<double> calc_vertical_gradient(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            std::vector<double> vertical_gradients = { -1, 0, 1,
                                                       -2, 0, 2,
                                                       -1, 0, 1 };

            return convolution<T>(image, image_widht, image_height, 3, 3, [&](std::vector<T> sub_image)
            {return dot_product(sub_image, vertical_gradients) / (double)8; });;
        }

        template<class T>
        std::vector<double> calc_edges(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            auto vertical_edge = calc_vertical_gradient(image, image_widht, image_height);

            auto horizontal_edge = calc_horizontal_gradient(image, image_widht, image_height);

            auto edges = calc_intensity(vertical_edge, horizontal_edge);

            return edges;
        }

        // inputs
        rs2::frame depth, ir, yuy, prev_yuy;
        stream_profile_interface* const _from;
        stream_profile_interface* const _to;

        // input/output
        rs2_extrinsics _extr;
        rs2_intrinsics _intr;
    };

    
} // namespace librealsense
