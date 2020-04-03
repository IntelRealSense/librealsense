// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <librealsense2/hpp/rs_internal.hpp>
#include <iostream>
//C:\Users\aangerma\sourc[i]e\repos\rgb2depth\rgb2depth\opencv - master\modules\gapi\include\opencv2\gapi\own
#include <opencv2/core/core.hpp>

#include <fstream>
#include <vector>
#include "math.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <map>
#include <algorithm>
#include <librealsense2/rsutil.h>


uint32_t width_z = 1024;
uint32_t height_z = 768;
auto width_yuy2 = 1920;
auto height_yuy2 = 1080;

auto max_optimization_iters = 50;


// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

    struct float2 { float x, y; float & operator [] (int i) { return (&x)[i]; } };
    struct float3 { float x, y, z; float & operator [] (int i) { return (&x)[i]; } };
   
    class auto_cal_algo
    {
    public:

        std::string z_file = "C:/work/librealsense/build/wrappers/opencv/imshow/7/Z_GrayScale_1024x768_00.00.28.0628_F9440687_0001.raw";//"LongRange/13/Z_GrayScale_1024x768_00.01.21.3573_F9440687_0001.raw";
        std::string i_file = "C:/work/librealsense/build/wrappers/opencv/imshow/7/I_GrayScale_1024x768_00.00.28.0628_F9440687_0001.raw";//"LongRange/13/I_GrayScale_1024x768_00.01.21.3573_F9440687_0000.raw";
        std::string yuy2_file = "C:/work/librealsense/build/wrappers/opencv/imshow/7/YUY2_YUY2_1920x1080_00.00.27.9425_F9440687_0000.raw";
        std::string yuy2_prev_file = "C:/work/librealsense/build/wrappers/opencv/imshow/7/YUY2_YUY2_1920x1080_00.00.27.9425_F9440687_0000.raw";

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
        };

        struct yuy2_frame_data
        {
            std::vector<uint8_t> yuy2_frame;
            std::vector<uint8_t> yuy2_prev_frame;
            std::vector<double> edges;
            std::vector<double> edges_IDT;
            std::vector<double> edges_IDTx;
            std::vector<double> edges_IDTy;
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

        struct
            std::map < direction, std::pair<int, int>> dir_map = { {deg_0, {1, 0}},
                                                                 {deg_45, {1, 1}},
                                                                 {deg_90, {0, 1}},
                                                                 {deg_135, {-1, 1}} };


        bool optimaize();

        std::vector<uint8_t> create_syntetic_y(int width, int height)
        {
            std::vector<uint8_t> res(width*height * 2, 0);

            for (auto i = 0;i < height; i++)
            {
                for (auto j = 0;j < width; j++)
                {
                    if (i >= width / 3 && i < width - width / 3 && j >= height / 3 && j < height - height / 3)
                    {
                        res[(i*width * 2 + j * 2)] = 200;
                    }
                }
            }
            return res;
        }

        template<class T>
        std::vector<T> get_image(std::string file, uint32_t width, uint32_t height, bool simulate = false)
        {
            std::ifstream f;
            f.open(file, std::ios::binary);
            if (!f.good())
                throw "invalid file";
            std::vector<T> data(width * height);
            f.read((char*)data.data(), width * height * sizeof(T));
            return data;
        }

        std::vector<uint16_t> get_depth_image(uint32_t width, uint32_t height)
        {
            auto data = get_image<uint16_t>(auto_cal_algo::z_file, width, height);
            return data;
        }

        std::vector<uint8_t> get_ir_image(uint32_t width, uint32_t height)
        {
            auto data = get_image<uint8_t>(auto_cal_algo::i_file, width, height);
            return data;
        }

        std::vector<uint16_t> get_yuy2_image(uint32_t width, uint32_t height)
        {
            auto data = get_image<uint16_t>(auto_cal_algo::yuy2_file, width, height);
            return data;
        }

        std::vector<uint16_t> get_yuy2_prev_image(uint32_t width, uint32_t height)
        {
            auto data = get_image<uint16_t>(auto_cal_algo::yuy2_prev_file, width, height);
            return data;
        }

    private:
        void zero_invalid_edges(z_frame_data& z_data, ir_frame_data ir_data);
        std::vector<double> get_direction_deg(std::vector<double> gradient_x, std::vector<double> gradient_y);
        std::vector<direction> get_direction(std::vector<double> gradient_x, std::vector<double> gradient_y);
        std::pair<uint32_t, uint32_t> get_prev_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);
        std::pair<uint32_t, uint32_t> get_next_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);
        std::vector<double> supressed_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        std::vector<uint16_t> get_closest_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        std::pair<std::vector<double>, std::vector<double>> calc_subpixels(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);
        z_frame_data preproccess_z(const ir_frame_data & ir_data, rs2_intrinsics depth_intrinsics, float depth_units);
        std::vector<double> blure_edges(std::vector<double> edges, uint32_t image_widht, uint32_t image_height);
        std::vector<uint8_t> get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh);
        yuy2_frame_data preprocess_yuy2_data(const rs2_intrinsics& rgb_intrinsics);
        ir_frame_data preprocess_ir();
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

        params _params;
    };

   
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

        cv::Mat edges_mat(height_yuy2, width_yuy2, CV_32F, vertical_edge.data());

        auto horizontal_edge = calc_horizontal_gradient(image, image_widht, image_height);

        auto edges = calc_intensity(vertical_edge, horizontal_edge);

        return edges;
    }
//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rsutil.h"

    auto_cal_algo::rotation_in_angles extract_angles_from_rotation(const float rotation[9])
    {
        auto_cal_algo::rotation_in_angles res;
        auto epsilon = 0.00001;
        res.alpha = atan2(-rotation[7], rotation[8]);
        res.beta = asin(rotation[6]);
        res.gamma = atan2(-rotation[3], rotation[0]);

        double rot[9];

        rot[0] = cos(res.beta)*cos(res.gamma);
        rot[1] = -cos(res.beta)*sin(res.gamma);
        rot[2] = sin(res.beta);
        rot[3] = cos(res.alpha)*sin(res.gamma) + cos(res.gamma)*sin(res.alpha)*sin(res.beta);
        rot[4] = cos(res.alpha)*cos(res.gamma) - sin(res.alpha)*sin(res.beta)*sin(res.gamma);
        rot[5] = -cos(res.beta)*sin(res.alpha);
        rot[6] = sin(res.alpha)*sin(res.gamma) - cos(res.alpha)*cos(res.gamma)*sin(res.beta);
        rot[7] = cos(res.gamma)*sin(res.alpha) + cos(res.alpha)*sin(res.beta)*sin(res.gamma);
        rot[8] = cos(res.alpha)*cos(res.beta);

        auto sum = 0;
        for (auto i = 0; i < 9; i++)
        {
            sum += rot[i] - rotation[i];
        }

        if ((abs(sum)) > epsilon)
            throw "No fit";
        return res;
    }

    auto_cal_algo::rotation extract_rotation_from_angles(const auto_cal_algo::rotation_in_angles & rot_angles)
    {
        auto_cal_algo::rotation res;

        auto sin_a = sin(rot_angles.alpha);
        auto sin_b = sin(rot_angles.beta);
        auto sin_g = sin(rot_angles.gamma);

        auto cos_a = cos(rot_angles.alpha);
        auto cos_b = cos(rot_angles.beta);
        auto cos_g = cos(rot_angles.gamma);

        res.rot[0] = cos_b * cos_g;
        res.rot[3] = -cos_b * sin_g;
        res.rot[6] = sin_b;
        res.rot[1] = cos_a * sin_g + cos_g * sin_a*sin_b;
        res.rot[4] = cos_a * cos_g - sin_a * sin_b*sin_g;
        res.rot[7] = -cos_b * sin_a;
        res.rot[2] = sin_a * sin_g - cos_a * cos_g*sin_b;
        res.rot[5] = cos_g * sin_a + cos_a * sin_b*sin_g;
        res.rot[8] = cos_a * cos_b;

        return res;
    }

    void auto_cal_algo::zero_invalid_edges(z_frame_data& z_data, ir_frame_data ir_data)
    {
        for (auto i = 0;i < ir_data.ir_edges.size();i++)
        {
            if (ir_data.ir_edges[i] <= _params.grad_ir_threshold || z_data.edges[i] <= _params.grad_z_threshold)
            {
                z_data.supressed_edges[i] = 0;
                z_data.subpixels_x[i] = 0;
                z_data.subpixels_y[i] = 0;
                z_data.closest[i] = 0;
            }
        }
    }

    std::vector< double> auto_cal_algo::get_direction_deg(std::vector<double> gradient_x, std::vector<double> gradient_y)
    {
#define PI 3.14159265
        std::vector<double> res(gradient_x.size(), deg_none);

        for (auto i = 0;i < gradient_x.size();i++)
        {
            int closest = -1;
            auto angle = atan2(gradient_y[i], gradient_x[i])* 180.f / PI;
            angle = angle < 0 ? 180 + angle : angle;
            auto dir = fmod(angle, 180);


            res[i] = dir;
        }
        return res;
    }

    std::vector< auto_cal_algo::direction> auto_cal_algo::get_direction(std::vector<double> gradient_x, std::vector<double> gradient_y)
    {
#define PI 3.14159265
        std::vector<direction> res(gradient_x.size(), deg_none);

        std::map<int, direction> angle_dir_map = { {0, deg_0}, {45,deg_45} , {90,deg_90}, {135,deg_135} };

        for (auto i = 0;i < gradient_x.size();i++)
        {
            int closest = -1;
            auto angle = atan2(gradient_y[i], gradient_x[i])* 180.f / PI;
            angle = angle < 0 ? 180 + angle : angle;
            auto dir = fmod(angle, 180);

            for (auto d : angle_dir_map)
            {
                closest = closest == -1 || abs(dir - d.first) < abs(dir - closest) ? d.first : closest;
            }
            res[i] = angle_dir_map[closest];
        }
        return res;
    }

    std::pair< uint32_t, uint32_t> auto_cal_algo::get_prev_index(auto_cal_algo::direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height)
    {
        auto d = dir_map[dir];

        auto edge_minus_idx = j - d.first;
        auto edge_minus_idy = i - d.second;


        edge_minus_idx = edge_minus_idx < 0 ? 0 : edge_minus_idx;
        edge_minus_idy = edge_minus_idy < 0 ? 0 : edge_minus_idy;

        return { edge_minus_idx, edge_minus_idy };
    }

    std::pair< uint32_t, uint32_t> auto_cal_algo::get_next_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height)
    {
        auto d = dir_map[dir];

        auto edge_plus_idx = j + d.first;
        auto edge_plus_idy = i + d.second;

        edge_plus_idx = edge_plus_idx < 0 ? 0 : edge_plus_idx;
        edge_plus_idy = edge_plus_idy < 0 ? 0 : edge_plus_idy;

        return { edge_plus_idx, edge_plus_idy };
    }

    std::vector< double> auto_cal_algo::supressed_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
    {
        std::vector< double> res(z_data.edges.begin(), z_data.edges.end());
        for (auto i = 0;i < height;i++)
        {
            for (auto j = 0;j < width;j++)
            {
                auto idx = i * width + j;

                auto edge = z_data.edges[idx];

                if (edge == 0)  continue;

                auto edge_prev_idx = get_prev_index(z_data.directions[idx], i, j, width, height);

                auto edge_next_idx = get_next_index(z_data.directions[idx], i, j, width, height);

                auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;

                auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;

                auto z_edge_plus = z_data.edges[edge_plus_idx];
                auto z_edge = z_data.edges[idx];
                auto z_edge_ninus = z_data.edges[edge_minus_idx];

                if (z_edge_ninus > z_edge || z_edge_plus > z_edge || ir_data.ir_edges[idx] <= _params.grad_ir_threshold || z_data.edges[idx] <= _params.grad_z_threshold)
                {
                    res[idx] = 0;
                }
            }
        }
        return res;
    }

    std::vector<uint16_t > auto_cal_algo::get_closest_edges(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
    {
        std::vector< uint16_t> z_closest;

        for (auto i = 0;i < height;i++)
        {
            for (auto j = 0;j < width;j++)
            {
                auto idx = i * width + j;

                auto edge = z_data.edges[idx];

                if (edge == 0)  continue;

                auto edge_prev_idx = get_prev_index(z_data.directions[idx], i, j, width, height);

                auto edge_next_idx = get_next_index(z_data.directions[idx], i, j, width, height);

                auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;

                auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;

                auto z_edge_plus = z_data.edges[edge_plus_idx];
                auto z_edge = z_data.edges[idx];
                auto z_edge_ninus = z_data.edges[edge_minus_idx];

                if (z_edge >= z_edge_ninus && z_edge >= z_edge_plus)
                {
                    if (ir_data.ir_edges[idx] > _params.grad_ir_threshold && z_data.edges[idx] > _params.grad_z_threshold)
                    {
                        z_closest.push_back(std::min(z_data.frame[edge_minus_idx], z_data.frame[edge_plus_idx]));
                    }
                }
            }
        }
        return z_closest;
    }

    std::pair<std::vector< double>, std::vector< double>> auto_cal_algo::calc_subpixels(const z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
    {
        std::vector< double> subpixels_x;
        std::vector< double> subpixels_y;

        for (auto i = 0;i < height;i++)
        {
            for (auto j = 0;j < width;j++)
            {
                auto idx = i * width + j;

                auto edge = z_data.edges[idx];

                if (edge == 0)  continue;

                auto edge_prev_idx = get_prev_index(z_data.directions[idx], i, j, width, height);

                auto edge_next_idx = get_next_index(z_data.directions[idx], i, j, width, height);

                auto edge_minus_idx = edge_prev_idx.second * width + edge_prev_idx.first;

                auto edge_plus_idx = edge_next_idx.second * width + edge_next_idx.first;

                auto z_edge_plus = z_data.edges[edge_plus_idx];
                auto z_edge = z_data.edges[idx];
                auto z_edge_ninus = z_data.edges[edge_minus_idx];

                auto dir = z_data.directions[idx];
                if (z_edge >= z_edge_ninus && z_edge >= z_edge_plus)
                {
                    if (ir_data.ir_edges[idx] > _params.grad_ir_threshold && z_data.edges[idx] > _params.grad_z_threshold)
                    {
                        auto fraq_step = double((-0.5f*double(z_edge_plus - z_edge_ninus)) / double(z_edge_plus + z_edge_ninus - 2 * z_edge));
                        subpixels_y.push_back(i + 1 + fraq_step * (double)dir_map[dir].second - 1);
                        subpixels_x.push_back(j + 1 + fraq_step * (double)dir_map[dir].first - 1);

                    }
                }

            }
        }
        return { subpixels_x, subpixels_y };
    }

    auto_cal_algo::z_frame_data auto_cal_algo::preproccess_z(const ir_frame_data& ir_data, rs2_intrinsics depth_intrinsics, float depth_units)
    {
        z_frame_data res;
        res.frame = get_depth_image(depth_intrinsics.width, depth_intrinsics.height);
        
        cv::Mat res_mat(height_z, width_z, CV_16U, res.frame.data());
        res.gradient_x = calc_vertical_gradient(res.frame, depth_intrinsics.width, depth_intrinsics.height);

        res.gradient_y = calc_horizontal_gradient(res.frame, depth_intrinsics.width, depth_intrinsics.height);

        res.edges = calc_intensity(res.gradient_x, res.gradient_y);

        res.directions = get_direction(res.gradient_x, res.gradient_y);
        res.direction_deg = get_direction_deg(res.gradient_x, res.gradient_y);
        res.supressed_edges = supressed_edges(res, ir_data, width_z, height_z);

        auto subpixels = calc_subpixels(res, ir_data, depth_intrinsics.width, depth_intrinsics.height);
        res.subpixels_x = subpixels.first;
        res.subpixels_y = subpixels.second;

        res.closest = get_closest_edges(res, ir_data, depth_intrinsics.width, depth_intrinsics.height);
       
        calculate_weights(res);

        auto vertices = subedges2vertices(res, depth_intrinsics, depth_units);
       
        return res;
    }

    std::vector<double> auto_cal_algo::blure_edges(std::vector<double> edges, uint32_t image_widht, uint32_t image_height)
    {
        std::vector<double> res(edges.begin(), edges.end());

        for (auto i = 0; i < image_height; i++)
            for (auto j = 0; j < image_widht; j++)
            {
                if (i == 0 && j == 0)
                    continue;
                else if (i == 0)
                    res[j] = std::max(res[j], res[j - 1] * _params.gamma);
                else if (j == 0)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[(i - 1)*image_widht + j] * _params.gamma);
                else
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], (std::max(res[i*image_widht + j - 1] * _params.gamma, res[(i - 1)*image_widht + j] * _params.gamma)));
            }

        for (int i = image_height - 1; i >= 0; i--)
            for (int j = image_widht - 1; j >= 0; j--)
            {
                if (i == image_height - 1 && j == image_widht - 1)
                    continue;
                else if (i == image_height - 1)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[i*image_widht + j + 1] * _params.gamma);
                else if (j == image_widht - 1)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[(i + 1)*image_widht + j] * _params.gamma);
                else
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], (std::max(res[i*image_widht + j + 1] * _params.gamma, res[(i + 1)*image_widht + j] * _params.gamma)));
            }

        for (int i = 0; i < image_height; i++)
            for (int j = 0; j < image_widht; j++)
                res[i*image_widht + j] = _params.alpha * edges[i*image_widht + j] + (1 - _params.alpha) * res[i*image_widht + j];
        return res;
    }

    std::vector<uint8_t> auto_cal_algo::get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh)
    {
        std::vector<uint8_t> res(yuy2_imagh.size(), 0);
        auto yuy2 = (uint8_t*)yuy2_imagh.data();
        for (auto i = 0;i < res.size(); i++)
            res[i] = yuy2[i * 2];

        return res;
    }

    auto_cal_algo::yuy2_frame_data auto_cal_algo::preprocess_yuy2_data(const rs2_intrinsics& rgb_intrinsics)
    {
        yuy2_frame_data res;

        auto yuy2_data = get_yuy2_image(rgb_intrinsics.width, rgb_intrinsics.height);
        res.yuy2_frame = get_luminance_from_yuy2(yuy2_data);
        cv::Mat res_mat(height_yuy2, width_yuy2, CV_8U, res.yuy2_frame.data());

        res.edges = calc_edges(res.yuy2_frame, rgb_intrinsics.width, rgb_intrinsics.height);
        cv::Mat edges_mat(height_yuy2, width_yuy2, CV_32F, res.edges.data());

        res.edges_IDT = blure_edges(res.edges, rgb_intrinsics.width, rgb_intrinsics.height);
        cv::Mat IDT_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDT.data());

        res.edges_IDTx = calc_vertical_gradient(res.edges_IDT, rgb_intrinsics.width, rgb_intrinsics.height);
        cv::Mat IDTx_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDTx.data());
        
        res.edges_IDTy = calc_horizontal_gradient(res.edges_IDT, rgb_intrinsics.width, rgb_intrinsics.height);
        cv::Mat IDTy_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDTy.data());
       
        auto yuy2_data_prev = get_yuy2_prev_image(rgb_intrinsics.width, rgb_intrinsics.height);
        res.yuy2_prev_frame = get_luminance_from_yuy2(yuy2_data_prev);

        return res;
    }

    auto_cal_algo::ir_frame_data auto_cal_algo::preprocess_ir()
    {
        auto data = get_ir_image(width_z, height_z);
        auto edges = calc_edges(data, width_z, height_z);

        return { data, edges };
    }

    std::vector<uint8_t> auto_cal_algo::get_logic_edges(std::vector<double> edges)
    {
        std::vector<uint8_t> logic_edges(edges.size(), 0);
        auto max = std::max_element(edges.begin(), edges.end());
        auto thresh = *max*_params.edge_thresh4_logic_lum;

        for (auto i = 0;i < edges.size(); i++)
        {
            logic_edges[i] = abs(edges[i]) > thresh ? 1 : 0;
        }
        return logic_edges;
    }

    bool auto_cal_algo::is_movement_in_images(const yuy2_frame_data& yuy)
    {
        auto logic_edges = get_logic_edges(yuy.edges);
        return true;
    }

    bool auto_cal_algo::is_scene_valid(yuy2_frame_data yuy)
    {
        return true;
    }

    std::vector<double> auto_cal_algo::calculate_weights(z_frame_data& z_data)
    {
        std::vector<double> res;
        for (auto i = 0;i < z_data.supressed_edges.size();i++)
        {
            if (z_data.supressed_edges[i])
                z_data.weights.push_back(
                    std::min(std::max((float)z_data.supressed_edges[i] - (float)_params.grad_z_min, 0.f), 
                                (float)_params.grad_z_max - (float)_params.grad_z_min));
        }

        return res;
    }

    std::vector<rs2_vertex> auto_cal_algo::subedges2vertices(z_frame_data& z_data, const rs2_intrinsics& intrin, double depth_units)
    {
        std::vector<rs2_vertex> res(z_data.subpixels_x.size());
        deproject_sub_pixel(res, intrin, z_data.subpixels_x.data(), z_data.subpixels_y.data(), z_data.closest.data(), depth_units);
        z_data.vertices = res;
        return res;
    }


    void auto_cal_algo::deproject_sub_pixel(std::vector<rs2_vertex>& points, const rs2_intrinsics& intrin, const double* x, const double* y, const uint16_t* depth, double depth_units)
    {
        auto ptr = (float*)points.data();
        for (int i = 0; i < points.size(); ++i)
        {
            const float pixel[] = { x[i], y[i] };
            rs2_deproject_pixel_to_point(ptr, &intrin, pixel, (*depth++)*depth_units);
            ptr += 3;
        }
    }

    std::pair< rs2_intrinsics, rs2_extrinsics> calib_to_intrinsics_extrinsics(auto_cal_algo::calib curr_calib)
    {
        auto rot = curr_calib.rot.rot;
        auto trans = curr_calib.trans;
        auto k = curr_calib.k_mat;
        auto coeffs = curr_calib.coeffs;

        rs2_intrinsics intrinsics = { curr_calib.width, curr_calib.height,
                                   k.ppx, k.ppy,
                                   k.fx,k.fy,
                                   curr_calib.model,
                                   {coeffs[0], coeffs[1], coeffs[2], coeffs[3], coeffs[4]} };

        rs2_extrinsics extr = { { rot[0], rot[1], rot[2], rot[3], rot[4], rot[5] ,rot[6], rot[7], rot[8] },
                                {trans.t1, trans.t2, trans.t3} };

        return { intrinsics, extr };
    }

    auto_cal_algo::calib intrinsics_extrinsics_to_calib(rs2_intrinsics intrin, rs2_extrinsics extrin)
    {
        auto_cal_algo::calib cal;
        auto rot = extrin.rotation;
        auto trans = extrin.translation;
        auto coeffs = intrin.coeffs;

        cal.rot = { rot[0], rot[1], rot[2], rot[3], rot[4], rot[5] ,rot[6], rot[7], rot[8] };
        cal.trans = { trans[0], trans[1], trans[2] };
        cal.k_mat = { intrin.fx, intrin.fy,intrin.ppx, intrin.ppy };
        cal.coeffs[0] = coeffs[0];
        cal.coeffs[1] = coeffs[1];
        cal.coeffs[2] = coeffs[2];
        cal.coeffs[3] = coeffs[3];
        cal.coeffs[4] = coeffs[4];
        cal.model = intrin.model;

        return cal;
    }

    std::vector<float2> get_texture_map(
        /*const*/ std::vector<rs2_vertex> points,
        auto_cal_algo::calib curr_calib)
    {
      
        auto ext = calib_to_intrinsics_extrinsics(curr_calib);

        auto intrinsics = ext.first;
        auto extr = ext.second;

        std::vector<float2> uv(points.size());
       
        for (auto i = 0; i < points.size(); ++i)
        {
            rs2_vertex p = {};
            rs2_transform_point_to_point(&p.xyz[0], &extr, &points[i].xyz[0]);
            float2 pixel = {};
            rs2_project_point_to_pixel(&pixel.x, &intrinsics, &p.xyz[0]);
            uv[i] = pixel;
        }

        return uv;
    }

    std::vector<double> biliniar_interp(std::vector<double> vals, uint32_t width, uint32_t height, std::vector<float2> uv)
    {
        std::vector<double> res(uv.size());

        for (auto i = 0;i < uv.size(); i++)
        {
            auto x = uv[i].x;
            auto x1 = floor(x);
            auto x2 = ceil(x);
            auto y = uv[i].y;
            auto y1 = floor(y);
            auto y2 = ceil(y);

            if (x1 < 0 || x1 >= width || x2 < 0 || x2 >= width ||
                y1 < 0 || y1 >= height || y2 < 0 || y2 >= height)
            {
                res[i] = std::numeric_limits<double>::max();
                continue;
            }

            // x1 y1    x2 y1
            // x1 y2    x2 y2

            // top_l    top_r
            // bot_l    bot_r

            auto top_l = vals[y1*width + x1];
            auto top_r = vals[y1*width + x2];
            auto bot_l = vals[y2*width + x1];
            auto bot_r = vals[y2*width + x2];

            double interp_x_top, interp_x_bot;
            if (x1 == x2)
            {
                interp_x_top = top_l;
                interp_x_bot = bot_l;
            }
            else
            {
                interp_x_top = ((double)(x2 - x) / (double)(x2 - x1))*(double)top_l + ((double)(x - x1) / (double)(x2 - x1))*(double)top_r;
                interp_x_bot = ((double)(x2 - x) / (double)(x2 - x1))*(double)bot_l + ((double)(x - x1) / (double)(x2 - x1))*(double)bot_r;
            }
          
            if (y1 == y2)
            {
                res[i] = interp_x_bot;
                continue;
            }
                
            auto interp_y_x = ((double)(y2 - y) / (double)(y2 - y1))*(double)interp_x_top + ((double)(y - y1) / (double)(y2 - y1))*(double)interp_x_bot;
            res[i] = interp_y_x;
        }
        return res;
    }

    

    auto_cal_algo::calib auto_cal_algo::get_calib_gradients(const z_frame_data& z_data)
    {

        return calib();
    }

    auto_cal_algo::optimaization_params auto_cal_algo::back_tracking_line_search(const z_frame_data & z_data, const yuy2_frame_data& yuy_data, auto_cal_algo::optimaization_params curr_params)
    {
        auto_cal_algo::optimaization_params new_params;

        auto grads_norm = curr_params.calib_gradients.normalize();
        auto normalized_grads = grads_norm / _params.normelize_mat;
        auto normalized_grads_norm = normalized_grads.get_norma();
        auto unit_grad = normalized_grads.normalize();

        auto t = calc_t(curr_params);
        auto step_size = calc_step_size(curr_params);

        curr_params.curr_calib.rot_angles = extract_angles_from_rotation(curr_params.curr_calib.rot.rot);
        auto movement = unit_grad * step_size;
        new_params.curr_calib = curr_params.curr_calib + movement;
        new_params.curr_calib.rot = extract_rotation_from_angles(new_params.curr_calib.rot_angles);

        auto uvmap = get_texture_map(z_data.vertices, curr_params.curr_calib);
        curr_params.cost = calc_cost(z_data, yuy_data, uvmap);

        uvmap = get_texture_map(z_data.vertices, new_params.curr_calib);
        new_params.cost = calc_cost(z_data, yuy_data, uvmap);

        auto iter_count = 0;
     
        while ((curr_params.cost - new_params.cost) >= step_size * t && abs(step_size) > _params.min_step_size && iter_count++ < _params.max_back_track_iters)
        {
            step_size = _params.tau*step_size;

            new_params.curr_calib = curr_params.curr_calib + unit_grad * step_size;
            new_params.curr_calib.rot = extract_rotation_from_angles(new_params.curr_calib.rot_angles);

            uvmap = get_texture_map(z_data.vertices, new_params.curr_calib);
            new_params.cost = calc_cost(z_data, yuy_data, uvmap);
            std::cout << "Current back tracking line search cost = " << new_params.cost << std::endl;
        }

        if (curr_params.cost - new_params.cost >= step_size * t)
        {
            new_params = curr_params;
        }

        return new_params;
    }

    double auto_cal_algo::calc_step_size(optimaization_params opt_params)
    {
        auto grads_norm = opt_params.calib_gradients.normalize();
        auto normalized_grads = grads_norm / _params.normelize_mat;
        auto normalized_grads_norm = normalized_grads.get_norma();
        auto unit_grad = normalized_grads.normalize();
        auto unit_grad_norm = unit_grad.get_norma();

        return _params.max_step_size*normalized_grads_norm / unit_grad_norm;
    }

    double auto_cal_algo::calc_t(optimaization_params opt_params)
    {
        auto grads_norm = opt_params.calib_gradients.normalize();
        auto normalized_grads = grads_norm / _params.normelize_mat;
        auto normalized_grads_norm = normalized_grads.get_norma();
        auto unit_grad = normalized_grads.normalize();

        auto t_vals = normalized_grads * -_params.control_param / unit_grad;
        return t_vals.sum();
    }

    
    double auto_cal_algo::calc_cost(const z_frame_data & z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv)
    {
        auto d_vals = biliniar_interp(yuy_data.edges_IDT, width_yuy2, height_yuy2, uv);
        double cost = 0;

        auto sum_of_elements = 0;
        for (auto i = 0;i < d_vals.size(); i++)
        {
            if (d_vals[i] != std::numeric_limits<double>::max())
            {
                sum_of_elements++;
                cost += d_vals[i] * z_data.weights[i];
            }
        }

        cost /= (double)sum_of_elements;
        return cost;
    }

    auto_cal_algo::calib auto_cal_algo::calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv, 
         const calib& curr_calib)
    {
        calib res;
        auto interp_IDT_x = biliniar_interp(yuy_data.edges_IDTx, width_yuy2, height_yuy2, uv);
        std::ofstream f;
        f.open("res");
        for (auto i = 0; i < interp_IDT_x.size(); i++)
        {
            f << uv[i].x<<" "<< uv[i].y << std::endl;
        }
       
        auto interp_IDT_y = biliniar_interp(yuy_data.edges_IDTy, width_yuy2, height_yuy2, uv);

        auto rc = calc_rc(z_data, yuy_data, curr_calib);
        
        auto intrin_extrin = calib_to_intrinsics_extrinsics(curr_calib);

        res.rot_angles = calc_rotation_gradients(z_data, yuy_data, interp_IDT_x, interp_IDT_y, intrin_extrin.first, intrin_extrin.second, rc.second, rc.first);
        res.trans = calc_translation_gradients(z_data, yuy_data, interp_IDT_x, interp_IDT_y, intrin_extrin.first, intrin_extrin.second, rc.second, rc.first);
        res.k_mat = calc_k_gradients(z_data, yuy_data, interp_IDT_x, interp_IDT_y, intrin_extrin.first, intrin_extrin.second, rc.second, rc.first);

        return res;
    }

    auto_cal_algo::translation auto_cal_algo::calc_translation_gradients(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto coefs = calc_translation_coefs(z_data, yuy_data, yuy_intrin, yuy_extrin, rc, xy);
        auto w = z_data.weights;
        
        translation sums = { 0 };
        auto sum_of_valids = 0;

        for (auto i = 0; i < coefs.x_coeffs.size(); i++)
        {
            if (interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max())
                continue;

            sum_of_valids++;

            sums.t1 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t1 + interp_IDT_y[i] * coefs.y_coeffs[i].t1);
            sums.t2 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t2 + interp_IDT_y[i] * coefs.y_coeffs[i].t2);
            sums.t3 += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].t3 + interp_IDT_y[i] * coefs.y_coeffs[i].t3);
        }

        translation averages{ (double)sums.t1 / (double)sum_of_valids,
            (double)sums.t2 / (double)sum_of_valids,
            (double)sums.t3 / (double)sum_of_valids };


        return averages;
    }

    auto_cal_algo::rotation_in_angles auto_cal_algo::calc_rotation_gradients(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto coefs = calc_rotation_coefs(z_data, yuy_data, yuy_intrin, yuy_extrin, rc, xy);
        auto w = z_data.weights;

        rotation_in_angles sums = { 0 };
        double sum_alpha = 0;
        double sum_beta = 0;
        double sum_gamma = 0;

        auto sum_of_valids = 0;

        for (auto i = 0; i < coefs.x_coeffs.size(); i++)
        {

            if (interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max())
                continue;

            sum_of_valids++;

           
            sum_alpha += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].alpha + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].alpha);
            sum_beta += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].beta + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].beta);
            sum_gamma += (double)w[i] * (double)((double)interp_IDT_x[i] * (double)coefs.x_coeffs[i].gamma + (double)interp_IDT_y[i] * (double)coefs.y_coeffs[i].gamma);
        }

        rotation_in_angles averages{ sum_alpha / (double)sum_of_valids,
            sum_beta / (double)sum_of_valids,
            sum_gamma / (double)sum_of_valids };

        return averages;
    }

    auto_cal_algo::k_matrix auto_cal_algo::calc_k_gradients(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<double> interp_IDT_x, std::vector<double> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto coefs = calc_k_gradients_coefs(z_data, yuy_data, yuy_intrin, yuy_extrin, rc, xy);
        auto w = z_data.weights;

        rotation_in_angles sums = { 0 };
        double sum_fx = 0;
        double sum_fy = 0;
        double sum_ppx = 0;
        double sum_ppy = 0;

        auto sum_of_valids = 0;

        for (auto i = 0; i < coefs.x_coeffs.size(); i++)
        {

            if (interp_IDT_x[i] == std::numeric_limits<double>::max() || interp_IDT_y[i] == std::numeric_limits<double>::max())
                continue;

            sum_of_valids++;

            sum_fx += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].fx + interp_IDT_y[i] * coefs.y_coeffs[i].fx);
            sum_fy += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].fy + interp_IDT_y[i] * coefs.y_coeffs[i].fy);
            sum_ppx += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].ppx + interp_IDT_y[i] * coefs.y_coeffs[i].ppx);
            sum_ppy += w[i] * (interp_IDT_x[i] * coefs.x_coeffs[i].ppy + interp_IDT_y[i] * coefs.y_coeffs[i].ppy);

        }
        k_matrix averages{ sum_fx / (double)sum_of_valids,
            sum_fy / (double)sum_of_valids,
            sum_ppx / (double)sum_of_valids,
            sum_ppy / (double)sum_of_valids };

        return averages;
    }

    std::pair< std::vector<float2>,std::vector<double>> auto_cal_algo::calc_rc(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib)
    {
        auto v = z_data.vertices;
       
        std::vector<float2> f1(z_data.vertices.size());
        std::vector<double> r2(z_data.vertices.size());
        std::vector<double> rc(z_data.vertices.size());

        auto intrin_extrin = calib_to_intrinsics_extrinsics(curr_calib);
        auto yuy_intrin = intrin_extrin.first;
        auto yuy_extrin = intrin_extrin.second;

        for (auto i = 0; i < z_data.vertices.size(); ++i)
        {
            rs2_vertex p = {};
            rs2_transform_point_to_point(&p.xyz[0], &yuy_extrin, &v[i].xyz[0]);
            f1[i].x = p.xyz[0] * yuy_intrin.fx + yuy_intrin.ppx*p.xyz[2];
            f1[i].x /= p.xyz[2];
            f1[i].x = (f1[i].x - yuy_intrin.ppx) / yuy_intrin.fx;

            f1[i].y = p.xyz[1] * yuy_intrin.fy + yuy_intrin.ppy*p.xyz[2];
            f1[i].y /= p.xyz[2];
            f1[i].y = (f1[i].y - yuy_intrin.ppy) / yuy_intrin.fy;

            r2[i] = f1[i].x*f1[i].x + f1[i].y*f1[i].y;
            rc[i] = 1 + yuy_intrin.coeffs[0] * r2[i] + yuy_intrin.coeffs[1] * r2[i] * r2[i] + yuy_intrin.coeffs[4] * r2[i] * r2[i] * r2[i];
        }

        return { f1,rc };
    }
    auto_cal_algo::translation auto_cal_algo::calculate_translation_y_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin)
    {
        auto_cal_algo::translation res;

        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto x = (double)v.xyz[0];
        auto y = (double)v.xyz[1];
        auto z = (double)v.xyz[2];

        auto exp1 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
        auto exp2 = fx * (double)r[2] * x + fx * (double)r[5] * y + fx * (double)r[8] * z + fx * (double)t[2];
        auto exp3 = fx * exp1 * exp1;
        auto exp4 = fy * (2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + y1 *
            (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*xy2 + 6 * (double)d[4] * x1*x2_y2))*exp2;

        res.t1 = exp4 / exp3;

        exp1 = rc + 2 * (double)d[3] * x1 + 6 * (double)d[2] * y1 + y1 *
            (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
        exp2 = -fy * (double)r[2] * x - fy * (double)r[5] * y - fy * (double)r[8] * z - fy * (double)t[2];
        exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

        res.t2 = -(exp1*exp2) / (exp3* exp3);

        exp1 = rc + 2 * (double)d[3] * x1 + 6 * (double)d[2] * y1 + y1 *            (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
        exp2 = fy * (double)r[1] * x + fy * (double)r[4] * y + fy * (double)r[7] * z + fy * (double)t[1];
        exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
        exp4 = 2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + y1 *
            (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*xy2 + 6 * (double)d[4] * x1*x2_y2);

        auto exp5 = fx * (double)r[0] * x + fx * (double)r[3] * y + fx * (double)r[6] * z + fx * (double)t[0];
        auto exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
        auto exp7 = fx * exp6 * exp6;

        res.t3 = -(exp1 * exp2) / (exp3 * exp3) - (fy*(exp4)*(exp5)) / exp7;

        return res;
    }

    auto_cal_algo::translation auto_cal_algo::calculate_translation_x_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto_cal_algo::translation res;

        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto x = (double)v.xyz[0];
        auto y = (double)v.xyz[1];
        auto z = (double)v.xyz[2];

        auto exp1 = rc + 6 * (double)d[3] * x1 + 2 * (double)d[2] * y1 + x1 *
            (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*(xy2)+6 * (double)d[4] * x1*(x2_y2));
        auto exp2 = fx * (double)r[2] * x + fx * (double)r[5] * y + fx * (double)r[8] * z + fx * (double)t[2];
        auto exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

        res.t1 = (exp1 * exp2) / (exp3 * exp3);

        auto exp4 = 2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + x1 *
            (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
        auto exp5 = -fy * (double)r[2] * x - fy * (double)r[5] * y - fy * (double)r[8] * z - fy * (double)t[2];
        auto exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

        res.t2 = -(fx*exp4 * exp5) / (fy*exp6 * exp6);

        exp1 = rc + 6 * (double)d[3] * x1 + 2 * (double)d[2] * y1 + x1
            * (2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*(xy2)+6 * (double)d[4] * x1*x2_y2);
        exp2 = fx * (double)r[0] * x + fx * (double)r[3] * y + fx * (double)r[6] * z + fx * (double)t[0];
        exp3 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];
        exp4 = fx * (2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 +
            x1 * (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*(xy2)+6 * (double)d[4] * y1*x2_y2));
        exp5 = +fy * (double)r[1] * x + fy * (double)r[4] * y + fy * (double)r[7] * z + fy * (double)t[1];
        exp6 = (double)r[2] * x + (double)r[5] * y + (double)r[8] * z + (double)t[2];

        res.t3 = -(exp1 * exp2) / (exp3 * exp3) - (exp4 * exp5) / (fy*exp6 * exp6);

        return res;

    }

    auto_cal_algo::coeffs<auto_cal_algo::rotation_in_angles> auto_cal_algo::calc_rotation_coefs(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto_cal_algo::coeffs<auto_cal_algo::rotation_in_angles> res;
        auto engles = extract_angles_from_rotation(yuy_extrin.rotation);
        auto v = z_data.vertices;
        res.x_coeffs.resize(v.size());
        res.y_coeffs.resize(v.size());

        for (auto i = 0; i < v.size(); i++)
        {
            res.x_coeffs[i].alpha = calculate_rotation_x_alpha_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.x_coeffs[i].beta = calculate_rotation_x_beta_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.x_coeffs[i].gamma = calculate_rotation_x_gamma_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);

            res.y_coeffs[i].alpha = calculate_rotation_y_alpha_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.y_coeffs[i].beta = calculate_rotation_y_beta_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.y_coeffs[i].gamma = calculate_rotation_y_gamma_coeff(engles, v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
        }

        return res;
    }

    auto_cal_algo::coeffs<auto_cal_algo::k_matrix> auto_cal_algo::calc_k_gradients_coefs(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto_cal_algo::coeffs<auto_cal_algo::k_matrix> res;
        auto v = z_data.vertices;
        res.x_coeffs.resize(v.size());
        res.y_coeffs.resize(v.size());

        for (auto i = 0; i < v.size(); i++)
        {
            res.x_coeffs[i] = calculate_k_gradients_x_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.y_coeffs[i] = calculate_k_gradients_y_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
        }

        return res;
    }

    auto_cal_algo::k_matrix auto_cal_algo::calculate_k_gradients_y_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto_cal_algo::k_matrix res;

        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;
        auto x = (double)v.xyz[0];
        auto y = (double)v.xyz[1];
        auto z = (double)v.xyz[2];

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        res.fx = (fy*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            *(r[0] * x + r[3] * y + r[6] * z + t[0]))
            / (fx*(x*r[2] + y * r[5] + z * r[8] + t[2]));

        res.ppx = (fy*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            *(r[2] * x + r[5] * y + r[8] * z + t[2]))
            / (fx*(x*r[2] + y * r[5] + z * (r[8]) + (t[2])));

        res.fy = ((r[1] * x + r[4] * y + r[7] * z + t[1])*
            (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)))
            / (x*r[2] + y * r[7] + z * r[8] + t[2]);

        res.ppy = ((r[2] * x + r[5] * y + r[8] * z + t[2])*
            (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)))
            / (x*r[2] + y * r[5] + z * r[8] + t[2]);

        return res;
    }

    auto_cal_algo::k_matrix auto_cal_algo::calculate_k_gradients_x_coeff(rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto_cal_algo::k_matrix res;

        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;
        auto x = (double)v.xyz[0];
        auto y = (double)v.xyz[1];
        auto z = (double)v.xyz[2];

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        res.fx = ((r[0] * x + r[3] * y + r[6] * z + t[0])
            *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))) 
            / (x*r[2] + y * r[5] + z * r[8] + t[2]);


        res.ppx = ((r[2] * x + r[5] * y + r[8] * z + t[2] * 1)
            *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            ) / (x*r[2] + y * (r[5]) + z * (r[8]) + (t[2]));


        res.fy = (fx*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))*
            (r[1] * x + r[4] * y + r[7] * z + t[1] * 1))
            / (fy*(x*(r[2]) + y * (r[5]) + z * (r[8]) + (t[2])));

        res.ppy = (fx*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))*(r[2] * x + r[5] * y + r[8] * z + t[2] * 1))
            / (fy*(x*(r[2]) + y * (r[5]) + z * (r[8]) + (t[2])));

        return res;
    }

    double auto_cal_algo::calculate_rotation_x_alpha_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto sin_a = (double)sin(rot_angles.alpha);
        auto sin_b = (double)sin(rot_angles.beta);
        auto sin_g = (double)sin(rot_angles.gamma);

        auto cos_a = (double)cos(rot_angles.alpha);
        auto cos_b = (double)cos(rot_angles.beta);
        auto cos_g = (double)cos(rot_angles.gamma);
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = (double)v.xyz[0];
        auto y = (double)v.xyz[1];
        auto z = (double)v.xyz[2];


        auto exp1 = z * (0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
            + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                + 0 * cos_b*cos_g)
            + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                - 0 * cos_b*sin_g)
            + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2]);

        auto res = (((x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
            - 1 * (cos_a*sin_g + cos_g * sin_a*sin_b)
            ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                - 1 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                ) + z * (0 * cos_a*cos_b + 1 * cos_b*sin_a)
            )*(z*(fx*sin_b + ppx * cos_a*cos_b - 0 * cos_b*sin_a)
                + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                    + ppx * (sin_a*sin_g - cos_a * cos_g*sin_b)
                    + fx * cos_b*cos_g)
                + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                    + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - fx * cos_b*sin_g)
                + 1 * (fx*t[0] + 0 * t[1] + ppx * t[2])
                ) - (x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                    - ppx * (cos_a*sin_g + cos_g * sin_a*sin_b)
                    ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                        - ppx * (cos_a*cos_g - sin_a * sin_b*sin_g)
                        ) + z * (0 * cos_a*cos_b + ppx * cos_b*sin_a)
                    )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                        + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                            + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                            + 0 * cos_b*cos_g)
                        + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                            + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                            - 0 * cos_b*sin_g)
                        + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                        ))
            *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            ) / (exp1*exp1) + (fx*((x*(0 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                - 1 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                ) + y * (0 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - 1 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                    ) + z * (0 * cos_a*cos_b + 1 * cos_b*sin_a)
                )*(z*(0 * sin_b + ppy * cos_a*cos_b - fy * cos_b*sin_a)
                    + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                        + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)
                        + 0 * cos_b*cos_g)
                    + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                        + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)
                        - 0 * cos_b*sin_g)
                    + 1 * (0 * t[0] + fy * t[1] + ppy * t[2])
                    ) - (x*(fy*(sin_a*sin_g - cos_a * cos_g*sin_b)
                        - ppy * (cos_a*sin_g + cos_g * sin_a*sin_b)
                        ) + y * (fy*(cos_g*sin_a + cos_a * sin_b*sin_g)
                            - ppy * (cos_a*cos_g - sin_a * sin_b*sin_g)
                            ) + z * (fy*cos_a*cos_b + ppy * cos_b*sin_a)
                        )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                            + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                                + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                                + 0 * cos_b*cos_g)
                            + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                                + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                                - 0 * cos_b*sin_g)
                            + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                            ))
                *(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))
                ) / (fy*(exp1*exp1));
        return res;
    }

    double auto_cal_algo::calculate_rotation_x_beta_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = yuy_intrin.ppx;
        auto ppy = yuy_intrin.ppy;
        auto fx = yuy_intrin.fx;
        auto fy = yuy_intrin.fy;

        auto sin_a = sin(rot_angles.alpha);
        auto sin_b = sin(rot_angles.beta);
        auto sin_g = sin(rot_angles.gamma);

        auto cos_a = cos(rot_angles.alpha);
        auto cos_b = cos(rot_angles.beta);
        auto cos_g = cos(rot_angles.gamma);
        auto x1 = xy.x;
        auto y1 = xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = v.xyz[0];
        auto y = v.xyz[1];
        auto z = v.xyz[2];

        auto exp1 = z * (cos_a*cos_b)+
            x * ((sin_a*sin_g - cos_a * cos_g*sin_b))
            + y * ((cos_g*sin_a + cos_a * sin_b*sin_g))
            + (t[2]);

        auto res = -(((z*(0 * cos_b - 1 * cos_a*sin_b + 0 * sin_a*sin_b)
            - x * (0 * cos_g*sin_b + 1 * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
            + y * (0 * sin_b*sin_g + 1 * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
            )*(z*(fx*sin_b + ppx * cos_a*cos_b - 0 * cos_b*sin_a)
                + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                    + ppx * (sin_a*sin_g - cos_a * cos_g*sin_b)
                    + fx * cos_b*cos_g)
                + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                    + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)
                    - fx * cos_b*sin_g)
                + 1 * (fx*t[0] + 0 * t[1] + ppx * t[2])
                ) - (z*(fx*cos_b - ppx * cos_a*sin_b + 0 * sin_a*sin_b)
                    - x * (fx*cos_g*sin_b + ppx * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
                    + y * (fx*sin_b*sin_g + ppx * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
                    )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                        + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                            + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                            + 0 * cos_b*cos_g)
                        + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                            + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                            - 0 * cos_b*sin_g)
                        + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])))
            *(rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            ) / (exp1* exp1) - (fx*((z*(0 * cos_b - 1 * cos_a*sin_b + 0 * sin_a*sin_b)
                - x * (0 * cos_g*sin_b + 1 * cos_a*cos_b*cos_g - 0 * cos_b*cos_g*sin_a)
                + y * (0 * sin_b*sin_g + 1 * cos_a*cos_b*sin_g - 0 * cos_b*sin_a*sin_g)
                )*(z*(0 * sin_b + ppy * cos_a*cos_b - fy * cos_b*sin_a)
                    + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                        + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)
                        + 0 * cos_b*cos_g)
                    + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                        + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)
                        - 0 * cos_b*sin_g)
                    + 1 * (0 * t[0] + fy * t[1] + ppy * t[2])
                    ) - (z*(0 * cos_b - ppy * cos_a*sin_b + fy * sin_a*sin_b)
                        - x * (0 * cos_g*sin_b + ppy * cos_a*cos_b*cos_g - fy * cos_b*cos_g*sin_a)
                        + y * (0 * sin_b*sin_g + ppy * cos_a*cos_b*sin_g - fy * cos_b*sin_a*sin_g)
                        )*(z*(0 * sin_b + 1 * cos_a*cos_b - 0 * cos_b*sin_a)
                            + x * (0 * (cos_a*sin_g + cos_g * sin_a*sin_b)
                                + 1 * (sin_a*sin_g - cos_a * cos_g*sin_b)
                                + 0 * cos_b*cos_g)
                            + y * (0 * (cos_a*cos_g - sin_a * sin_b*sin_g)
                                + 1 * (cos_g*sin_a + cos_a * sin_b*sin_g)
                                - 0 * cos_b*sin_g)
                            + 1 * (0 * t[0] + 0 * t[1] + 1 * t[2])
                            ))*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))
                ) / (fy*(exp1*exp1));

        return res;
    }

    double auto_cal_algo::calculate_rotation_x_gamma_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto sin_a = (double)sin(rot_angles.alpha);
        auto sin_b = (double)sin(rot_angles.beta);
        auto sin_g = (double)sin(rot_angles.gamma);

        auto cos_a = (double)cos(rot_angles.alpha);
        auto cos_b = (double)cos(rot_angles.beta);
        auto cos_g = (double)cos(rot_angles.gamma);
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = v.xyz[0];
        auto y = v.xyz[1];
        auto z = v.xyz[2];

        auto exp1 = z * cos_a*cos_b +
            x * (sin_a*sin_g - cos_a * cos_g*sin_b) +
            y * (cos_g*sin_a + cos_a * sin_b*sin_g) +
            t[2];

        auto res = (
            ((y*(sin_a*sin_g - cos_a * cos_g*sin_b) - x * (cos_g*sin_a + cos_a * sin_b*sin_g))*
            (z*(fx*sin_b + ppx * cos_a*cos_b) +
                x * (ppx*(sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) +
                y * (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) +
                (fx*t[0] + ppx * t[2])) -
                (y*(ppx* (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) -
                    x * (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g))*
                    (z*(cos_a*cos_b) + x * (sin_a*sin_g - cos_a * cos_g*sin_b) +
                        y * (cos_g*sin_a + cos_a * sin_b*sin_g) + t[2]))*
                        (rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
            )
            /
            (exp1* exp1) + (fx*((y*(sin_a*sin_g - cos_a * cos_g*sin_b) -
                x * (cos_g*sin_a + cos_a * sin_b*sin_g))*
                (z*(ppy*cos_a*cos_b - fy * cos_b*sin_a) +
                    x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y *
                    (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy *
                    (cos_g*sin_a + cos_a * sin_b*sin_g)) +
                        (fy * t[1] + ppy * t[2])) -
                    (y*(fy*(cos_a*sin_g + cos_g * sin_a*sin_b) +
                        ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) - x *
                        (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)))*
                        (z*cos_a*cos_b + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
                            y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2]))*(2 * d[2] * x1 + 2 * d[3] * y1 + x1 *
                            (2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2)) / (fy*exp1*exp1));

        return res;
    }

    double auto_cal_algo::calculate_rotation_y_alpha_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto sin_a = (double)sin(rot_angles.alpha);
        auto sin_b = (double)sin(rot_angles.beta);
        auto sin_g = (double)sin(rot_angles.gamma);

        auto cos_a = (double)cos(rot_angles.alpha);
        auto cos_b = (double)cos(rot_angles.beta);
        auto cos_g = (double)cos(rot_angles.gamma);
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

       /* x1 = 1;
        y1 = 1;*/

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = v.xyz[0];
        auto y = v.xyz[1];
        auto z = v.xyz[2];


        auto exp1 = z * (cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
            y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2];

        auto res = (((x*(-(cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (-1 * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z *
            (cos_b*sin_a))*(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a) + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) +
                ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) +
                    ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)) + (fy * t[1] + ppy * t[2])) - (x*(fy*(sin_a*sin_g - cos_a * cos_g*sin_b) -
                        ppy * (cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (fy*(cos_g*sin_a + cos_a * sin_b*sin_g) -
                            ppy * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z * (fy*cos_a*cos_b + ppy * cos_b*sin_a))*
                            (z*(cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g) - 0 * cos_b*sin_g) + (t[2])))*
            (rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))) /
            (exp1*exp1) + (fy*((x*(-(cos_a*sin_g + cos_g * sin_a*sin_b)) + y * (-(cos_a*cos_g - sin_a * sin_b*sin_g)) +
                z * (cos_b*sin_a))*(z*(fx*sin_b + ppx * cos_a*cos_b) + x * (ppx*(sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) + y *
                (ppx*(cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) + (fx*t[0] + ppx * t[2])) - (x*(-ppx * (cos_a*sin_g + cos_g * sin_a*sin_b)) +
                    y * (-ppx * (cos_a*cos_g - sin_a * sin_b*sin_g)) + z * (ppx*cos_b*sin_a))*(z*(cos_a*cos_b - 0 * cos_b*sin_a) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) +
                        y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))) / (fx*(exp1*exp1));

        return res;
    }

    double auto_cal_algo::calculate_rotation_y_beta_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto sin_a = (double)sin(rot_angles.alpha);
        auto sin_b = (double)sin(rot_angles.beta);
        auto sin_g = (double)sin(rot_angles.gamma);

        auto cos_a = (double)cos(rot_angles.alpha);
        auto cos_b = (double)cos(rot_angles.beta);
        auto cos_g = (double)cos(rot_angles.gamma);
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = v.xyz[0];
        auto y = v.xyz[1];
        auto z = v.xyz[2];

        auto exp1 = z * ( cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b))
            + y * ((cos_g*sin_a + cos_a * sin_b*sin_g))+ (t[2]);

        auto res = -(((z*(-cos_a * sin_b) - x * (cos_a*cos_b*cos_g)
            + y * (cos_a*cos_b*sin_g))*(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a)
                + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))
                + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g))
                + (fy * t[1] + ppy * t[2])) - (z*(0 * cos_b - ppy * cos_a*sin_b + fy * sin_a*sin_b)
                    - x * (ppy * cos_a*cos_b*cos_g - fy * cos_b*cos_g*sin_a) + y * (ppy * cos_a*cos_b*sin_g - fy * cos_b*sin_a*sin_g))*
                    (z*(cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2]))
            *(rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))) /
            (exp1*exp1) - (fy*((z*(-cos_a * sin_b) - x * (cos_a*cos_b*cos_g) + y * (cos_a*cos_b*sin_g))*(z*(fx*sin_b + ppx * cos_a*cos_b)
                + x * (ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) + y * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g)
                + (fx*t[0] + ppx * t[2])) - (z*(fx*cos_b - ppx * cos_a*sin_b) - x * (fx*cos_g*sin_b + ppx * cos_a*cos_b*cos_g) + y
                    * (fx*sin_b*sin_g + ppx * cos_a*cos_b*sin_g))*(z*(cos_a*cos_b) + x * (sin_a*sin_g - cos_a * cos_g*sin_b) + y
                        * (cos_g*sin_a + cos_a * sin_b*sin_g) + t[2]))*(2 * d[2] * x1 + 2 * d[3] * y1 + y1 *
                        (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))) / (fx*(exp1*exp1));

        return res;

    }

    double auto_cal_algo::calculate_rotation_y_gamma_coeff(rotation_in_angles rot_angles, rs2_vertex v, double rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto r = yuy_extrin.rotation;
        auto t = yuy_extrin.translation;
        auto d = yuy_intrin.coeffs;
        auto ppx = (double)yuy_intrin.ppx;
        auto ppy = (double)yuy_intrin.ppy;
        auto fx = (double)yuy_intrin.fx;
        auto fy = (double)yuy_intrin.fy;

        auto sin_a = (double)sin(rot_angles.alpha);
        auto sin_b = (double)sin(rot_angles.beta);
        auto sin_g = (double)sin(rot_angles.gamma);

        auto cos_a = (double)cos(rot_angles.alpha);
        auto cos_b = (double)cos(rot_angles.beta);
        auto cos_g = (double)cos(rot_angles.gamma);
        auto x1 = (double)xy.x;
        auto y1 = (double)xy.y;

        auto x2 = x1 * x1;
        auto y2 = y1 * y1;
        auto xy2 = x2 + y2;
        auto x2_y2 = xy2 * xy2;

        auto x = v.xyz[0];
        auto y = v.xyz[1];
        auto z = v.xyz[2];

        auto exp1 = z * (cos_a*cos_b) + x * (+(sin_a*sin_g - cos_a * cos_g*sin_b))
            + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + t[2];

        auto res = (((y*(+ (sin_a*sin_g - cos_a * cos_g*sin_b))- x * ((cos_g*sin_a + cos_a * sin_b*sin_g)))
                  *(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a) + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                  + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))+ y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                  + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g))
                  + (fy * t[1] + ppy * t[2])) - (y*(fy*(cos_a*sin_g + cos_g * sin_a*sin_b)+ ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))
                  - x * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)))*(z*(cos_a*cos_b )
                  + x * ((sin_a*sin_g - cos_a * cos_g*sin_b))+ y * ( + (cos_g*sin_a + cos_a * sin_b*sin_g))+ (t[2])))
                  *(rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2))) 
                  / (exp1*exp1) + (fy*((y*(+ (sin_a*sin_g - cos_a * cos_g*sin_b))- x * (+ (cos_g*sin_a + cos_a * sin_b*sin_g)))
                  *(z*(fx*sin_b + ppx * cos_a*cos_b )+ x * ( ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g)
                  + y * ( + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)- fx * cos_b*sin_g)+ (fx*t[0] + ppx * t[2])) 
                  - (y*(ppx * (sin_a*sin_g - cos_a * cos_g*sin_b)+ fx * cos_b*cos_g)- x 
                  * ( + ppx * (cos_g*sin_a + cos_a * sin_b*sin_g)- fx * cos_b*sin_g))*(z*(cos_a*cos_b )
                  + x * ((sin_a*sin_g - cos_a * cos_g*sin_b))+ y * ((cos_g*sin_a + cos_a * sin_b*sin_g))+ (t[2])))
                  *(2 * d[2] * x1 + 2 * d[3] * y1 + y1 * (2 * d[0] * x1 + 4 * d[1] * x1*(xy2)+6 * d[4] * x1*x2_y2))
                  ) / (fx*(exp1*exp1));

        return res;
    }

    auto_cal_algo::coeffs<auto_cal_algo::translation> auto_cal_algo::calc_translation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin, const std::vector<double>& rc, const std::vector<float2>& xy)
    {
        auto_cal_algo::coeffs<auto_cal_algo::translation> res;

        auto v = z_data.vertices;
        res.y_coeffs.resize(v.size());
        res.x_coeffs.resize(v.size());

        for (auto i = 0; i < rc.size(); i++)
        {
            res.y_coeffs[i] = calculate_translation_y_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.x_coeffs[i] = calculate_translation_x_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);

        }
       
        return res;
    }

    std::pair<auto_cal_algo::calib, double> auto_cal_algo::calc_cost_and_grad(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib)
    {
        auto uvmap = get_texture_map(z_data.vertices, curr_calib);
        
        std::ofstream f;
        f.open("uvmap");
        for (auto i = 0; i < uvmap.size(); i++)
        {
            f << uvmap[i].x<< " " << uvmap[i].y << std::endl;

        }
        auto cost = calc_cost(z_data, yuy_data, uvmap);
        auto grad = calc_gradients(z_data, yuy_data, uvmap, curr_calib);
        return { grad, cost };
    }

    bool auto_cal_algo::optimaize()
    {
        rs2_intrinsics rgb_intrinsics = { width_yuy2, height_yuy2,
                                    959.33734, 568.44531,
                                    1355.8387,1355.1364,
                                    RS2_DISTORTION_BROWN_CONRADY, {0.14909270, -0.49516717, -0.00067913462, 0.0010808484, 0.42839915} };

        rs2_intrinsics depth_intrin = { width_z, height_z,
           529.27344, 402.32031,
           731.27344,731.97656,
           RS2_DISTORTION_NONE, {0, 0, 0, 0, 0} };

        float depth_units = 0.25f;

        auto yuy_data = preprocess_yuy2_data(rgb_intrinsics);
        auto ir_data = preprocess_ir();
        auto z_data = preproccess_z(ir_data, depth_intrin, depth_units);


        rs2_extrinsics extrinsics = { { 0.99970001, -0.021156590, 0.012339020,
                                       0.021360161, 0.99963391, -0.016606549,
                                      -0.011983165, 0.016865131, 0.99978596 },
                                       1.1025798, 13.548738, -6.8803735 };



        optimaization_params params_orig;
        params_orig.curr_calib = intrinsics_extrinsics_to_calib(rgb_intrinsics, extrinsics);

        auto optimized = false;

        auto cost = calc_cost_and_grad(z_data, yuy_data, params_orig.curr_calib);
        std::cout << "Original cost = " << cost.second << std::endl;

        optimaization_params params_curr = params_orig;

        for (auto i = 0; i < _params.max_optimization_iters; i++)
        {
            auto res = calc_cost_and_grad(z_data, yuy_data, params_curr.curr_calib);

            params_curr.cost = res.second;
            params_curr.calib_gradients = res.first;
           
            auto new_params = back_tracking_line_search(z_data, yuy_data, params_curr);
            if ((new_params.curr_calib - params_curr.curr_calib).get_norma() < _params.min_rgb_mat_delta)
                break;

            if(abs(new_params.cost- params_curr.cost)< _params.min_cost_delta)
                break;

            params_curr = new_params;
            std::cout << "Current optimaized cost = " << params_curr.cost << std::endl;

            optimized = true;
        }
        std::cout << "Optimaized cost = " << params_curr.cost << std::endl;

        return optimized;
    }

int main()
{
    auto_cal_algo auto_cal;
    auto_cal.optimaize();


    std::cout << "Hello World!\n";

    /* std::string res_file("F9440842_scenexp1/binFiles/Z_edgeSupressed_480x640_single_00.bin");
     auto res = get_image<double>(res_file, width_z, height_z);

     cv::Mat res_mat(height_z, width_z, CV_32F, res.data());

     for (auto i = 0;i < valid_z_edges.size(); i++)
     {
         auto val = valid_z_edges[i];
         auto val1 = res[i];
         if (abs(val - val1) > 0.0001)
             std::cout << "err";
     }*/

     /*cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);// Create a window for display.

     cv::imshow("Display window", blured_mat);
     cv::waitKey(0);*/
}

void auto_cal_algo::calib::copy_coefs(calib & obj)
{
    obj.coeffs[0] = this->coeffs[0];
    obj.coeffs[1] = this->coeffs[1];
    obj.coeffs[2] = this->coeffs[2];
    obj.coeffs[3] = this->coeffs[3];
    obj.coeffs[4] = this->coeffs[4];

    obj.model = this->model;
}

auto_cal_algo::calib auto_cal_algo::calib::operator*(double step_size)
{
    calib res;
    res.k_mat.fx = this->k_mat.fx * step_size;
    res.k_mat.fy = this->k_mat.fy * step_size;
    res.k_mat.ppx = this->k_mat.ppx * step_size;
    res.k_mat.ppy = this->k_mat.ppy *step_size;

    res.rot_angles.alpha = this->rot_angles.alpha *step_size;
    res.rot_angles.beta = this->rot_angles.beta *step_size;
    res.rot_angles.gamma = this->rot_angles.gamma *step_size;

    res.trans.t1 = this->trans.t1 *step_size;
    res.trans.t2 = this->trans.t2 * step_size;
    res.trans.t3 = this->trans.t3 *step_size;

    copy_coefs(res);

    return res;
}

auto_cal_algo::calib auto_cal_algo::calib::operator/(double factor)
{
    return (*this)*(1.f / factor);
}

auto_cal_algo::calib auto_cal_algo::calib::operator+(const calib & c)
{
    calib res;
    res.k_mat.fx = this->k_mat.fx + c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy + c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx + c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy + c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha + c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta + c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma + c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 + c.trans.t1;
    res.trans.t2 = this->trans.t2 + c.trans.t2;
    res.trans.t3 = this->trans.t3 + c.trans.t3;

    copy_coefs(res);

    return res;
}

auto_cal_algo::calib auto_cal_algo::calib::operator-(const calib & c)
{
    calib res;
    res.k_mat.fx = this->k_mat.fx - c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy - c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx - c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy - c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha - c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta - c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma - c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 - c.trans.t1;
    res.trans.t2 = this->trans.t2 - c.trans.t2;
    res.trans.t3 = this->trans.t3 - c.trans.t3;

    copy_coefs(res);

    return res;
}

auto_cal_algo::calib auto_cal_algo::calib::operator/(const calib & c)
{
    calib res;
    res.k_mat.fx = this->k_mat.fx / c.k_mat.fx;
    res.k_mat.fy = this->k_mat.fy / c.k_mat.fy;
    res.k_mat.ppx = this->k_mat.ppx / c.k_mat.ppx;
    res.k_mat.ppy = this->k_mat.ppy / c.k_mat.ppy;

    res.rot_angles.alpha = this->rot_angles.alpha / c.rot_angles.alpha;
    res.rot_angles.beta = this->rot_angles.beta / c.rot_angles.beta;
    res.rot_angles.gamma = this->rot_angles.gamma / c.rot_angles.gamma;

    res.trans.t1 = this->trans.t1 / c.trans.t1;
    res.trans.t2 = this->trans.t2 / c.trans.t2;
    res.trans.t3 = this->trans.t3 / c.trans.t3;

    copy_coefs(res);

    return res;
}

double auto_cal_algo::calib::get_norma()
{
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                        trans.t1, trans.t2, trans.t3,
                        k_mat.ppx, k_mat.ppy, k_mat.fx, k_mat.fy };

    auto grads_norm = 0.f;

    for (auto i = 0; i < grads.size(); i++)
    {
        grads_norm += grads[i] * grads[i];
    }
    grads_norm = sqrtf(grads_norm);

    return grads_norm;
}

double auto_cal_algo::calib::sum()
{
    auto res = 0.f;
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                       trans.t1, trans.t2, trans.t3,
                       k_mat.ppx, k_mat.ppy, k_mat.fx, k_mat.fy, };


    for (auto i = 0; i < grads.size(); i++)
    {
        res += grads[i];
    }

    return res;
}

auto_cal_algo::calib auto_cal_algo::calib::normalize()
{
    std::vector<double> grads = { rot_angles.alpha,rot_angles.beta, rot_angles.gamma,
                        trans.t1, trans.t2, trans.t3,
                        k_mat.ppx, k_mat.ppy, k_mat.fx, k_mat.fy };

    auto norma = get_norma();

    std::vector<double> res_grads(grads.size());

    for (auto i = 0; i < grads.size(); i++)
    {
        res_grads[i] = grads[i] / norma;
    }

    calib res;
    res.rot_angles = { res_grads[0], res_grads[1], res_grads[2] };
    res.trans = { res_grads[3], res_grads[4], res_grads[5] };
    res.k_mat = { res_grads[6], res_grads[7], res_grads[8], res_grads[9] };

    return res;
}

auto_cal_algo::params::params()
{
    normelize_mat.k_mat = { 0.354172020000000, 0.265703050000000,1.001765500000000, 1.006649100000000 };
    normelize_mat.rot_angles = { 1508.94780000000, 1604.94300000000,649.384340000000 };
    normelize_mat.trans = { 0.913008390000000, 0.916982890000000, 0.433054570000000 };
}