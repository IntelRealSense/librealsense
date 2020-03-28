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

float gamma = 0.98;
float alpha = 0.333333333333333;
float grad_ir_threshold = 3.5f; // Ignore pixels with IR grad of less than this
int grad_z_threshold = 25;  //Ignore pixels with Z grad of less than this
float edge_thresh4_logic_lum = 0.1;

uint32_t width_z = 1024;
uint32_t height_z = 768;
auto width_yuy2 = 1920;
auto height_yuy2 = 1080;
auto min_weighted_edge_per_section_depth = 50;
auto num_of_sections_for_edge_distribution_x = 2;
auto num_of_sections_for_edge_distribution_y = 2;
auto grad_z_min = 25.f; // Ignore pixels with Z grad of less than this
auto grad_z_max = 1000.f;
auto max_optimization_iters = 50;


// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

    struct float2 { float x, y; float & operator [] (int i) { return (&x)[i]; } };
    struct float3 { float x, y, z; float & operator [] (int i) { return (&x)[i]; } };

    class auto_cal_algo
    {
    public:

        std::string z_file = "C:/work/librealsense/build/wrappers/opencv/imshow/2/Z_GrayScale_1024x768_0000.raw";//"LongRange/13/Z_GrayScale_1024x768_00.01.21.3573_F9440687_0001.raw";
        std::string i_file = "C:/work/librealsense/build/wrappers/opencv/imshow/2/I_GrayScale_1024x768_0000.raw";//"LongRange/13/I_GrayScale_1024x768_00.01.21.3573_F9440687_0000.raw";
        std::string yuy2_file = "C:/work/librealsense/build/wrappers/opencv/imshow/2/YUY2_YUY2_1920x1080_0000.raw";
        std::string yuy2_prev_file = "C:/work/librealsense/build/wrappers/opencv/imshow/2/YUY2_YUY2_1920x1080_0000.raw";

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
            std::vector<float> ir_edges;
        };

        struct z_frame_data
        {
            std::vector<uint16_t> frame;
            std::vector<float> gradient_x;
            std::vector<float> gradient_y;
            std::vector<float> edges;
            std::vector<float> supressed_edges;
            std::vector<direction> directions;
            std::vector<float> subpixels_x;
            std::vector<float> subpixels_y;
            std::vector< uint16_t> closest;
            std::vector<float> weights;
            std::vector<float> direction_deg;
            std::vector<rs2_vertex> vertices;
        };

        struct yuy2_frame_data
        {
            std::vector<uint8_t> yuy2_frame;
            std::vector<uint8_t> yuy2_prev_frame;
            std::vector<float> edges;
            std::vector<float> edges_IDT;
            std::vector<float> edges_IDTx;
            std::vector<float> edges_IDTy;
        };

        struct translation_gradients
        {
            float t1;
            float t2;
            float t3;
        };

        struct rotation_gradients
        {
            float alpha;
            float beta;
            float gamma;
        };

        struct k_gradients
        {
            float ppx;
            float ppy;
            float fx;
            float fy;
        };

        struct calib_gradients
        {
            translation_gradients trans_grads;
            rotation_gradients rot_grads;
            k_gradients k_grads;
        };

        template <class T>
        struct coeffs
        {
            std::vector<T> x_coeffs;
            std::vector<T> y_coeffs;
        };

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

        std::vector<float> get_direction_deg(std::vector<float> gradient_x, std::vector<float> gradient_y);

        std::vector<direction> get_direction(std::vector<float> gradient_x, std::vector<float> gradient_y);

        std::vector<float> calc_intensity(std::vector<float> image1, std::vector<float> image2);

        std::pair<uint32_t, uint32_t> get_prev_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);

        std::pair<uint32_t, uint32_t> get_next_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height);

        std::vector<float> supressed_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);

        std::vector<uint16_t> get_closest_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);

        std::pair<std::vector<float>, std::vector<float>> calc_subpixels(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height);

        z_frame_data preproccess_z(const ir_frame_data & ir_data);

        std::vector<float> blure_edges(std::vector<float> edges, uint32_t image_widht, uint32_t image_height, float gamma, float alpha);

        std::vector<uint8_t> get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh);

        yuy2_frame_data preprocess_yuy2_data();

        ir_frame_data get_ir_data();

        std::vector<uint8_t> get_logic_edges(std::vector<float> edges);

        bool is_movement_in_images(const yuy2_frame_data & yuy);

        bool is_scene_valid(yuy2_frame_data yuy);

        std::vector<float> calculate_weights(z_frame_data& z_data);

        std::vector <rs2_vertex> subedges2vertices(z_frame_data& z_data, const rs2_intrinsics& intrin, float depth_units);

        calib_gradients get_calib_gradients(const z_frame_data& z_data);

        std::pair< auto_cal_algo::calib_gradients, float> calc_cost_and_grad(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);

        float calc_cost(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv);

        calib_gradients calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin);

        translation_gradients calc_translation_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<float> interp_IDT_x, std::vector<float> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy);

        rotation_gradients calc_rotation_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, std::vector<float> interp_IDT_x, std::vector<float> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy);

        std::pair< std::vector<float2>, std::vector<float>> calc_rc(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin);

        coeffs<translation_gradients> calc_translation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy);

        translation_gradients auto_cal_algo::calculate_translation_y_coeff(rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);

        translation_gradients auto_cal_algo::calculate_translation_x_coeff(rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);

        coeffs<rotation_gradients> calc_rotation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy);
        
        float calculate_rotation_x_alpha_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        float calculate_rotation_x_beta_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        float calculate_rotation_x_gamma_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        float calculate_rotation_y_alpha_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        float calculate_rotation_y_beta_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);
        float calculate_rotation_y_gamma_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin);

        rotation_gradients extract_angles_from_rotation(const float rotation[9]);
        
            template<class T>
        std::vector<float> calc_gradients(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            auto vertical_edge = calc_vertical_gradient(image, image_widht, image_height);

            cv::Mat edges_mat(height_yuy2, width_yuy2, CV_32F, vertical_edge.data());

            auto horizontal_edge = calc_horizontal_gradient(image, image_widht, image_height);

            auto edges = calc_intensity(vertical_edge, horizontal_edge);

            return edges;
        }

        template<class T>
        float dot_product(std::vector<T> sub_image, std::vector<float> mask)
        {
            //check that sizes are equal
            float res = 0;

            for (auto i = 0; i < sub_image.size(); i++)
            {
                res += sub_image[i] * mask[i];
            }

            return res;
        }

        template<class T>
        std::vector<float> convolution(std::vector<T> image, uint32_t image_widht, uint32_t image_height, uint32_t mask_widht, uint32_t mask_height, std::function<float(std::vector<T> sub_image)> convolution_operation)
        {
            std::vector<float> res(image.size(), 0);


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
                        /* auto max = *std::max_element(sub_image.begin(), sub_image.end());
                         res1 = (float)max;*/

                    auto mid = (i + mask_height / 2) * image_widht + j + mask_widht / 2;
                    res[mid] = convolution_operation(sub_image);
                    
                }
            }
            return res;
        }

        template<class T>
        std::vector<float> calc_vertical_gradient(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            std::vector<float> vertical_gradients = { -1, 0, 1,
                                                       -2, 0, 2,
                                                       -1, 0, 1 };

            return convolution<T>(image, image_widht, image_height, 3, 3, [&](std::vector<T> sub_image) 
            {
                auto prod = dot_product(sub_image, vertical_gradients);
                auto res = prod / 8.f;
                return res;
            }
            );
        }


        template<class T>
        std::vector<float> calc_horizontal_gradient(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            std::vector<float> horizontal_gradients = { -1, -2, -1,
                                                          0,  0,  0,
                                                          1,  2,  1 };

            return convolution<T>(image, image_widht, image_height, 3, 3, [&](std::vector<T> sub_image) {return dot_product(sub_image, horizontal_gradients) / 8.f;});
        }

        void deproject_sub_pixel(std::vector<rs2_vertex>& points, const rs2_intrinsics & intrin, const float * x, const float * y, const uint16_t * depth, float depth_units);

        float gamma = 0.98;
        float alpha = 0.333333333333333;
        float grad_ir_threshold = 3.5f; // Ignore pixels with IR grad of less than this
        int grad_z_threshold = 25;  //Ignore pixels with Z grad of less than this
        float edge_thresh4_logic_lum = 0.1;

        uint32_t width_z = 1024;
        uint32_t height_z = 768;
        uint32_t width_yuy2 = 1920;
        uint32_t height_yuy2 = 1080;
        float min_weighted_edge_per_section_depth = 50;
        float num_of_sections_for_edge_distribution_x = 2;
        float num_of_sections_for_edge_distribution_y = 2;
        float grad_z_min = 25.f; // Ignore pixels with Z grad of less than this
        float grad_z_max = 1000.f;
        float max_optimization_iters = 50;

    };

//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../include/librealsense2/rsutil.h"


    void auto_cal_algo::zero_invalid_edges(z_frame_data& z_data, ir_frame_data ir_data)
    {
        for (auto i = 0;i < ir_data.ir_edges.size();i++)
        {
            if (ir_data.ir_edges[i] <= grad_ir_threshold || z_data.edges[i] <= grad_z_threshold)
            {
                z_data.supressed_edges[i] = 0;
                z_data.subpixels_x[i] = 0;
                z_data.subpixels_y[i] = 0;
                z_data.closest[i] = 0;
            }
        }
    }

    std::vector< float> auto_cal_algo::get_direction_deg(std::vector<float> gradient_x, std::vector<float> gradient_y)
    {
#define PI 3.14159265
        std::vector<float> res(gradient_x.size(), deg_none);

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

    std::vector< auto_cal_algo::direction> auto_cal_algo::get_direction(std::vector<float> gradient_x, std::vector<float> gradient_y)
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

    std::vector<float> auto_cal_algo::calc_intensity(std::vector<float> imagexp1, std::vector<float> imagexp2)
    {
        std::vector<float> res(imagexp1.size(), 0);
        //check that sizes are equal
        for (auto i = 0; i < imagexp1.size(); i++)
        {
            res[i] = sqrt(pow(imagexp1[i], 2) + pow(imagexp2[i] , 2));
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

    std::vector< float> auto_cal_algo::supressed_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
    {
        std::vector< float> res(z_data.edges.begin(), z_data.edges.end());
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

                if (z_edge_ninus > z_edge || z_edge_plus > z_edge || ir_data.ir_edges[idx] <= grad_ir_threshold || z_data.edges[idx] <= grad_z_threshold)
                {
                    res[idx] = 0;
                }
            }
        }
        return res;
    }

    std::vector<uint16_t > auto_cal_algo::get_closest_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
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
                    if (ir_data.ir_edges[idx] > grad_ir_threshold && z_data.edges[idx] > grad_z_threshold)
                    {
                        z_closest.push_back(std::min(z_data.frame[edge_minus_idx], z_data.frame[edge_plus_idx]));
                    }
                }
            }
        }
        return z_closest;
    }

    std::pair<std::vector< float>, std::vector< float>> auto_cal_algo::calc_subpixels(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
    {
        std::vector< float> subpixels_x;
        std::vector< float> subpixels_y;

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


                //subpixels_x[idx] = fraq_step;
                auto dir = z_data.directions[idx];
                if (z_edge >= z_edge_ninus && z_edge >= z_edge_plus)
                {
                    if (ir_data.ir_edges[idx] > grad_ir_threshold && z_data.edges[idx] > grad_z_threshold)
                    {
                        auto fraq_step = float((-0.5f*float(z_edge_plus - z_edge_ninus)) / float(z_edge_plus + z_edge_ninus - 2 * z_edge));
                        z_data.subpixels_y.push_back(i + 1 + fraq_step * (float)dir_map[dir].second - 1);
                        z_data.subpixels_x.push_back(j + 1 + fraq_step * (float)dir_map[dir].first - 1);

                    }
                }

            }
        }
        return { subpixels_x, subpixels_y };
    }

    auto_cal_algo::z_frame_data auto_cal_algo::preproccess_z(const ir_frame_data& ir_data)
    {
        z_frame_data res;
        res.frame = get_depth_image(width_z, height_z);

        res.gradient_x = calc_vertical_gradient(res.frame, width_z, height_z);

        res.gradient_y = calc_horizontal_gradient(res.frame, width_z, height_z);

        res.edges = calc_intensity(res.gradient_x, res.gradient_y);

        res.directions = get_direction(res.gradient_x, res.gradient_y);
        res.direction_deg = get_direction_deg(res.gradient_x, res.gradient_y);
        res.supressed_edges = supressed_edges(res, ir_data, width_z, height_z);

        auto subpixels = calc_subpixels(res, ir_data, width_z, height_z);

        /*res.subpixels_x = subpixels.first;
        res.subpixels_y = subpixels.second;*/

        res.closest = get_closest_edges(res, ir_data, width_z, height_z);
        calculate_weights(res);
        //zero_invalid_edges(res, ir_data);

        rs2_intrinsics depth_intrin = { width_z, height_z,
            529.27344, 402.32031,
            731.27344,731.97656,
           RS2_DISTORTION_INVERSE_BROWN_CONRADY, {0, 0, 0, 0, 0} };

        float depth_units = 0.25f;
        auto vertices = subedges2vertices(res, depth_intrin, depth_units);

        //std::sort(vertices.begin(), vertices.end(), [](rs2_vertex v1, rs2_vertex v2) {return v1.xyz[0] < v2.xyz[0];});
        return res;
    }

    std::vector<float> auto_cal_algo::blure_edges(std::vector<float> edges, uint32_t image_widht, uint32_t image_height, float gamma, float alpha)
    {
        std::vector<float> res(edges.begin(), edges.end());

        for (auto i = 0; i < image_height; i++)
            for (auto j = 0; j < image_widht; j++)
            {
                if (i == 0 && j == 0)
                    continue;
                else if (i == 0)
                    res[j] = std::max(res[j], res[j - 1] * gamma);
                else if (j == 0)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[(i - 1)*image_widht + j] * gamma);
                else
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], (std::max(res[i*image_widht + j - 1] * gamma, res[(i - 1)*image_widht + j] * gamma)));
            }

        for (int i = image_height - 1; i >= 0; i--)
            for (int j = image_widht - 1; j >= 0; j--)
            {
                if (i == image_height - 1 && j == image_widht - 1)
                    continue;
                else if (i == image_height - 1)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[i*image_widht + j + 1] * gamma);
                else if (j == image_widht - 1)
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], res[(i + 1)*image_widht + j] * gamma);
                else
                    res[i*image_widht + j] = std::max(res[i*image_widht + j], (std::max(res[i*image_widht + j + 1] * gamma, res[(i + 1)*image_widht + j] * gamma)));
            }

        for (int i = 0; i < image_height; i++)
            for (int j = 0; j < image_widht; j++)
                res[i*image_widht + j] = alpha * edges[i*image_widht + j] + (1 - alpha) * res[i*image_widht + j];
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

    auto_cal_algo::yuy2_frame_data auto_cal_algo::preprocess_yuy2_data()
    {
        yuy2_frame_data res;

        auto yuy2_data = get_yuy2_image(width_yuy2, height_yuy2);
        res.yuy2_frame = get_luminance_from_yuy2(yuy2_data);
        cv::Mat res_mat(height_yuy2, width_yuy2, CV_8U, res.yuy2_frame.data());

        res.edges = calc_gradients(res.yuy2_frame, width_yuy2, height_yuy2);
        cv::Mat edges_mat(height_yuy2, width_yuy2, CV_32F, res.edges.data());

        res.edges_IDT = blure_edges(res.edges, width_yuy2, height_yuy2, gamma, alpha);
        cv::Mat IDT_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDT.data());

        res.edges_IDTx = calc_vertical_gradient(res.edges_IDT, width_yuy2, height_yuy2);
        cv::Mat IDTx_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDTx.data());

        res.edges_IDTy = calc_horizontal_gradient(res.edges_IDT, width_yuy2, height_yuy2);
        cv::Mat IDTy_mat(height_yuy2, width_yuy2, CV_32F, res.edges_IDTy.data());

        auto yuy2_data_prev = get_yuy2_prev_image(width_yuy2, height_yuy2);
        res.yuy2_prev_frame = get_luminance_from_yuy2(yuy2_data_prev);

        return res;
    }

    auto_cal_algo::ir_frame_data auto_cal_algo::get_ir_data()
    {
        auto data = get_ir_image(width_z, height_z);
        auto edges = calc_gradients(data, width_z, height_z);

        return { data, edges };
    }

    std::vector<uint8_t> auto_cal_algo::get_logic_edges(std::vector<float> edges)
    {
        std::vector<uint8_t> logic_edges(edges.size(), 0);
        auto max = std::max_element(edges.begin(), edges.end());
        auto thresh = *max*edge_thresh4_logic_lum;

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

    std::vector<float> auto_cal_algo::calculate_weights(z_frame_data& z_data)
    {
        std::vector<float> res;
        for (auto i = 0;i < z_data.supressed_edges.size();i++)
        {
            if (z_data.supressed_edges[i])
                z_data.weights.push_back(std::min(std::max(z_data.supressed_edges[i] - grad_z_min, 0.f), grad_z_max - grad_z_min));
        }

        return res;
    }

    std::vector<rs2_vertex> auto_cal_algo::subedges2vertices(z_frame_data& z_data, const rs2_intrinsics& intrin, float depth_units)
    {
        std::vector<rs2_vertex> res(z_data.subpixels_x.size());
        deproject_sub_pixel(res, intrin, z_data.subpixels_x.data(), z_data.subpixels_y.data(), z_data.closest.data(), depth_units);
        z_data.vertices = res;
        return res;
    }


    void auto_cal_algo::deproject_sub_pixel(std::vector<rs2_vertex>& points, const rs2_intrinsics& intrin, const float* x, const float* y, const uint16_t* depth, float depth_units)
    {
        auto ptr = (float*)points.data();
        for (int i = 0; i < points.size(); ++i)
        {
            const float pixel[] = { x[i], y[i] };
            rs2_deproject_pixel_to_point(ptr, &intrin, pixel, (*depth++)*depth_units);
            ptr += 3;
        }
    }

    std::vector<float2> get_texture_map(
        /*const*/ std::vector<rs2_vertex> points,
        const rs2_intrinsics &intrinsics,
        const rs2_extrinsics& extr)
    {
        std::vector<float2> uv(points.size());
        //std::sort(points.begin(), points.end(), [](rs2_vertex v1, rs2_vertex v2) {return v1.xyz[0] < v2.xyz[0];});

        for (auto i = 0; i < points.size(); ++i)
        {
            rs2_vertex p = {};
            rs2_transform_point_to_point(&p.xyz[0], &extr, &points[i].xyz[0]);
             //auto tex_xy = project_to_texcoord(&mapped_intr, trans);
            // Store intermediate results for poincloud filters
            float2 pixel = {};
            rs2_project_point_to_pixel(&pixel.x, &intrinsics, &p.xyz[0]);
            uv[i] = pixel;
        }

        return uv;
    }

    std::vector<float> biliniar_interp(std::vector<float> vals, uint32_t width, uint32_t height, std::vector<float2> uv)
    {
        std::vector<float> res(uv.size());

        for (auto i = 0;i < uv.size(); i++)
        {
            auto x = uv[i].x;
            auto x1 = floor(x);
            auto x2 = ceil(x);
            auto y = uv[i].y;
            auto y1 = floor(y);
            auto y2 = ceil(y);

            if (x1 < 0 || x1 > width || x2 < 0 || x2 >= width ||
                y1 < 0 || y1 > height || y2 < 0 || y2 >= height)
            {
                res[i] = -1;
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

            auto interp_x_top = ((double)(x2 - x) / (double)(x2 - x1))*(double)top_l + ((double)(x - x1) / (double)(x2 - x1))*(double)top_r;
            auto interp_x_bot = ((double)(x2 - x) / (double)(x2 - x1))*(double)bot_l + ((double)(x - x1) / (double)(x2 - x1))*(double)bot_r;

            auto interp_y_x = ((double)(y2 - y) / (double)(y2 - y1))*(double)interp_x_top + ((double)(y - y1) / (double)(y2 - y1))*(double)interp_x_bot;

            res[i] = interp_y_x;
        }
        return res;
    }

    

    auto_cal_algo::calib_gradients auto_cal_algo::get_calib_gradients(const z_frame_data& z_data)
    {

        return calib_gradients();
    }

    
    float auto_cal_algo::calc_cost(const z_frame_data & z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv)
    {
        auto d_vals = biliniar_interp(yuy_data.edges_IDT, width_yuy2, height_yuy2, uv);
        auto cost = 0.f;

        auto sum_of_elements = 0;
        for (auto i = 0;i < d_vals.size(); i++)
        {
            if (d_vals[i] != -1)
            {
                sum_of_elements++;
                cost += d_vals[i] * z_data.weights[i];
            }
        }

        cost /= (float)sum_of_elements;
        return cost;
    }

    auto_cal_algo::calib_gradients auto_cal_algo::calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin)
    {
        auto c = uv;
        //std::sort(c.begin(), c.end(), [](float2 v1, float2 v2) {return v1.x < v2.x;});
        auto interp_IDT_x = biliniar_interp(yuy_data.edges_IDTx, width_yuy2, height_yuy2, uv);
       //std::sort(interp_IDT_x.begin(), interp_IDT_x.end());

        auto interp_IDT_y = biliniar_interp(yuy_data.edges_IDTy, width_yuy2, height_yuy2, uv);
       //std::sort(interp_IDT_y.begin(), interp_IDT_y.end());
        auto rc = calc_rc(z_data, yuy_data, yuy_intrin, yuy_extrin);
        
        calc_rotation_gradients(z_data, yuy_data, interp_IDT_x, interp_IDT_y, yuy_intrin, yuy_extrin, rc.second, rc.first);
        calc_translation_gradients(z_data, yuy_data, interp_IDT_x, interp_IDT_y, yuy_intrin, yuy_extrin, rc.second, rc.first);

        return calib_gradients();
    }

    auto_cal_algo::translation_gradients auto_cal_algo::calc_translation_gradients(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<float> interp_IDT_x, std::vector<float> interp_IDT_y, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy)
    {
        calc_translation_coefs(z_data, yuy_data, yuy_intrin, yuy_extrin, rc, xy);
        return translation_gradients();
    }

    auto_cal_algo::rotation_gradients auto_cal_algo::calc_rotation_gradients(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, std::vector<float> interp_IDT_x, std::vector<float> interp_IDT_y, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy)
    {
        calc_rotation_coefs(z_data, yuy_data, yuy_intrin, yuy_extrin, rc, xy);
        return rotation_gradients();
    }

    std::pair< std::vector<float2>,std::vector<float>> auto_cal_algo::calc_rc(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto v = z_data.vertices;
        //Krgb = params.Krgb;
        //A = params.rgbPmat;
        std::vector<float2> f1(z_data.vertices.size());
        std::vector<float> r2(z_data.vertices.size());
        std::vector<float> rc(z_data.vertices.size());

        //std::sort(v.begin(), v.end(), [](rs2_vertex v1, rs2_vertex v2) {return v1.xyz[0] < v2.xyz[0]; });
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
        //std::sort(f1.begin(), f1.end(), [](float3 v1, float3 v2) {return v1.x < v2.x;});
                //std::sort(f1.begin(), f1.end(), [](float3 v1, float3 v2) {return v1.y < v2.y;});
                //std::sort(r2.begin(), r2.end());
                //std::sort(rc[i].begin(), rc[i].end());

        return { f1,rc };
    }
    auto_cal_algo::translation_gradients auto_cal_algo::calculate_translation_y_coeff(rs2_vertex v, float rc, float2 xy, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin)
    {
        auto_cal_algo::translation_gradients res;

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

    auto_cal_algo::translation_gradients auto_cal_algo::calculate_translation_x_coeff(rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto_cal_algo::translation_gradients res;

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

    auto_cal_algo::coeffs<auto_cal_algo::rotation_gradients> auto_cal_algo::calc_rotation_coefs(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy)
    {
        auto_cal_algo::coeffs<auto_cal_algo::rotation_gradients> res;
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
        std::sort(res.x_coeffs.begin(), res.x_coeffs.end(), [](auto_cal_algo::rotation_gradients  r1, auto_cal_algo::rotation_gradients   r2) {return r1.gamma < r2.gamma; });
        std::sort(res.y_coeffs.begin(), res.y_coeffs.end(), [](auto_cal_algo::rotation_gradients  r1, auto_cal_algo::rotation_gradients   r2) {return r1.gamma < r2.gamma; });

        return res;
    }

    float auto_cal_algo::calculate_rotation_x_alpha_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

        auto exp1 = ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g;
        auto exp2 = -(cos_a*sin_g + cos_g * sin_a*sin_b);
        auto exp3 = y * (-1 * (cos_a*cos_g - sin_a * sin_b*sin_g));
        auto exp4 = z * (cos_b*sin_a);
        auto exp5 = z * (fx*sin_b + ppx * cos_a*cos_b);
        auto exp6 = -ppx * (cos_a*sin_g + cos_g * sin_a*sin_b);
        auto exp7 = -ppx * (cos_a*cos_g - sin_a * sin_b*sin_g);
        auto exp8 = ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g;
        auto exp9 = fx * (double)t[0] + ppx * (double)t[2];
        auto exp10 = z * (ppx*cos_b*sin_a);
        auto exp11 = z * (cos_a*cos_b);
        auto exp12 = x * (sin_a*sin_g - cos_a * cos_g*sin_b);
        auto exp13 = y * (cos_g*sin_a + cos_a * sin_b*sin_g);
        auto exp48 = 6 * (double)d[3] * x1;
        auto exp49 = 2 * (double)d[2] * y1;
        auto exp50 = 2 * (double)d[0] * x1 + 4 * (double)d[1] * x1*xy2 + 6 * (double)d[4] * x1*x2_y2;
        auto exp15 = (double)rc + exp48 + exp49 + x1 * exp50;
        auto exp16 = z * cos_a*cos_b;
        auto exp17 = x * (sin_a*sin_g - cos_a * cos_g*sin_b);
        auto exp18 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto exp20 = x * (-(cos_a*sin_g + cos_g * sin_a*sin_b));
        auto exp21 = y * (-(cos_a*cos_g - sin_a * sin_b*sin_g));
        auto exp22 = z * cos_b*sin_a;
        auto exp23 = z * (ppy*cos_a*cos_b - fy * cos_b*sin_a);
        auto exp24 = fy * (cos_a*sin_g + cos_g * sin_a*sin_b) +
            ppy * (sin_a*sin_g - cos_a * cos_g*sin_b);
        auto exp53 = x * exp24;
        auto exp25 = y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) +
            ppy * (cos_g*sin_a + cos_a * sin_b*sin_g));
        auto exp26 = fy * (sin_a*sin_g - cos_a * cos_g*sin_b) -
            ppy * (cos_a*sin_g + cos_g * sin_a*sin_b);
        auto exp27 = fy * (double)t[1] + ppy * (double)t[2];
        auto exp28 = y * (fy*(cos_g*sin_a + cos_a * sin_b*sin_g) -
            ppy * (cos_a*cos_g - sin_a * sin_b*sin_g));
        auto exp29 = x * (exp26)+exp28 + z
            * (fy*cos_a*cos_b + ppy * cos_b*sin_a);
        auto exp30 = x * (sin_a*sin_g - cos_a * cos_g*sin_b);
        auto exp31 = y * (cos_g*sin_a + cos_a * sin_b*sin_g);
        auto exp32 = z * (cos_a*cos_b) + exp30 + exp31 + ((double)t[2]);
        auto exp33 = fx * ((exp20 + exp21 + exp22)
            *(exp23 + exp53 + exp25 + exp27) - exp29 * exp32);
        auto exp34 = 2 * (double)d[2] * x1 + 2 * (double)d[3] * y1 + x1
            * (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
        auto exp35 = x * (sin_a*sin_g - cos_a * cos_g*sin_b);
        auto exp36 = y * (cos_g*sin_a + cos_a * sin_b*sin_g);
        auto exp37 = z * (cos_a*cos_b) + exp35 + exp36 + t[2];
        auto exp44 = x * exp2 + exp3 + exp4;
        auto exp45 = exp5 + x * exp1 + y * exp8 + exp9;
        auto exp46 = x * (exp6)+y * (exp7)+exp10;
        auto exp47 = exp11 + exp12 + exp13 + (double)t[2];
        auto exp51 = exp44 * exp45;
        auto exp52 = exp46 * exp47;
        auto exp38 = exp51 - exp52;
        auto exp39 = exp16 + exp17 + y * exp18 + (double)t[2];
        auto exp40 = fy * exp37 * exp37;
        auto exp41 = exp38 * exp15;
        auto exp42 = exp39 * exp39;
        auto exp43 = exp33 * exp34;

        auto res = exp41 / exp42 + exp43 / exp40;

        return res;
    }

    float auto_cal_algo::calculate_rotation_x_beta_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

        auto exp1 =cos_a*cos_b*cos_g;
        auto exp2 = z*(-cos_a*sin_b);
        auto exp3 = cos_a * cos_b*sin_g;
        auto exp4 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto exp5 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto exp6 = ppx * exp5 - fx * cos_b*sin_g;
        auto exp7 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto exp8 = fx * sin_b + ppx * cos_a*cos_b;
        auto exp9 = ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) +
            fx * cos_b*cos_g;
        auto exp10 = fx * t[0] + ppx * t[2];
        auto exp11 = z * exp8 + x * exp9 + y * exp6 + exp10;
        auto  exp12 = fx * cos_g*sin_b + ppx * cos_a*cos_b*cos_g;
        auto  exp13 = fx * cos_b - ppx * cos_a*sin_b;
        auto  exp14 = fx * sin_b*sin_g + ppx * cos_a*cos_b*sin_g;
        auto exp15 = z * exp13 - x * exp12 + y * exp14;
        auto exp16 = exp2 - x * (exp1)+y * (exp3);
        auto exp17 = cos_a * cos_b;
        auto  exp18 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto  exp19 = sin_a * sin_g - cos_a * cos_g*sin_b;
        auto  exp20 = exp19;
        auto  exp21 = exp19;
        auto  exp22 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto  exp23 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto exp24 = exp23;
        auto  exp25 = t[2];
        auto  exp26 = z * exp17 + x * exp20 + y * exp24 + exp25;
        auto  exp27 = exp16 * exp11 - exp15 * exp26;
        auto  exp28 = 2 * d[0] * x1 + 4 * d[1] * x1*xy2+6 * d[4] * x1*x2_y2;
        auto  exp29 = rc + 6 * d[3] * x1 + 2 * d[2] * y1 + x1 * exp28;
        auto  exp30 = exp27 * exp29;
        auto  exp31 = cos_a * cos_b;
        auto  exp32 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto  exp33 = sin_a * sin_g - cos_a * cos_g*sin_b;
        auto  exp34 = exp33;
        auto  exp35 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto  exp36 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto  exp37 = exp36;
        auto  exp38 = t[2];
        auto  exp39 = -cos_a * sin_b;
        auto exp40 = cos_a * cos_b*cos_g;
        auto  exp41 = cos_a * cos_b*sin_g;
        auto  exp42 = ppy * cos_a*cos_b - fy * cos_b*sin_a;
        auto  exp43 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto  exp44 = sin_a * sin_g - cos_a * cos_g*sin_b;
        auto  exp45 =0;
        auto  exp46 = z * exp31 + x * exp34 + y * exp37 + exp38;
        auto  exp47 = z * exp39 - x * exp40 + y * exp41;
        auto  exp48 = fy * exp43 + ppy * exp44 + exp45;
        auto  exp50 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto  exp51 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto  exp52 = fy * exp50 + ppy * exp51;
        auto  exp53 = fy * t[1] + ppy * t[2];
        auto exp54 = -ppy * cos_a*sin_b + fy * sin_a*sin_b;
        auto exp55 = ppy * cos_a*cos_b*cos_g - fy * cos_b*cos_g*sin_a;
        auto  exp56 = ppy * cos_a*cos_b*sin_g - fy * cos_b*sin_a*sin_g;
        auto  exp57 = z * exp42 + x * exp48 + y * exp52 + exp53;
        auto  exp58 = z * exp54 - x * exp55 + y * exp56;
        auto  exp59 = cos_a * cos_b;
        auto  exp60 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto  exp61 = sin_a * sin_g - cos_a * cos_g*sin_b;
        auto  exp62 = exp61;
        auto  exp63 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto  exp64 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto  exp65 = 0;
        auto  exp66 = exp64 - exp65;
        auto  exp67 = t[2];
        auto  exp68 = z * exp59 + x * exp62 + y * exp66 + 1 * exp67;
        auto  exp69 = exp47 * exp57 - exp58 * exp68;
        auto  exp70 = 2 * d[0] * y1 + 4 * d[1] * y1*xy2 + 6 * d[4] * y1*x2_y2;
        auto  exp71 = 2 * d[2] * x1 + 2 * d[3] * y1 + x1 * exp70;
        auto  exp72 = fx * exp69*exp71;
        auto  exp73 = cos_a * cos_b;
        auto  exp74 = cos_a * sin_g + cos_g * sin_a*sin_b;
        auto  exp75 = sin_a * sin_g - cos_a * cos_g*sin_b;
        auto  exp76 = exp75;
        auto  exp77 = cos_a * cos_g - sin_a * sin_b*sin_g;
        auto  exp78 = cos_g * sin_a + cos_a * sin_b*sin_g;
        auto  exp79 = exp78;
        auto  exp80 = t[2];
        auto  exp81 = z * exp73 + x * exp76 + y * exp79 + exp80;
        auto  exp82 = fy * exp81*exp81;
        auto res = -exp30 / (exp46* exp46) - exp72/ exp82;

        return res;
    }

    float auto_cal_algo::calculate_rotation_x_gamma_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

    float auto_cal_algo::calculate_rotation_y_alpha_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

    float auto_cal_algo::calculate_rotation_y_beta_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

    float auto_cal_algo::calculate_rotation_y_gamma_coeff(rotation_gradients rot_angles, rs2_vertex v, float rc, float2 xy, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
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

    auto_cal_algo::rotation_gradients auto_cal_algo::extract_angles_from_rotation(const float rotation[9])
    {
        auto_cal_algo::rotation_gradients res;
        auto epsilon = 0.00001;
        res.alpha = atan2(-rotation[7], rotation[8]);
        res.beta = asin(rotation[6]);
        res.gamma = atan2(-rotation[3], rotation[0]);

        float rot[9];

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

        if((abs(sum)) > epsilon)
            throw "No fit";
        return res;
    }

    auto_cal_algo::coeffs<auto_cal_algo::translation_gradients> auto_cal_algo::calc_translation_coefs(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const rs2_intrinsics& yuy_intrin, const rs2_extrinsics& yuy_extrin, const std::vector<float>& rc, const std::vector<float2>& xy)
    {
        auto_cal_algo::coeffs<auto_cal_algo::translation_gradients> res;

        auto v = z_data.vertices;
        res.y_coeffs.resize(v.size());
        res.x_coeffs.resize(v.size());

        for (auto i = 0; i < rc.size(); i++)
        {
            res.y_coeffs[i] = calculate_translation_y_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);
            res.x_coeffs[i] = calculate_translation_x_coeff(v[i], rc[i], xy[i], yuy_intrin, yuy_extrin);

        }
        std::sort(res.x_coeffs.begin(), res.x_coeffs.end(), [](auto_cal_algo::translation_gradients v1, auto_cal_algo::translation_gradients v2) {return v1.t1 < v2.t1; });

        return res;
    }

    std::pair<auto_cal_algo::calib_gradients, float> auto_cal_algo::calc_cost_and_grad(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const rs2_intrinsics & yuy_intrin, const rs2_extrinsics & yuy_extrin)
    {
        auto uvmap = get_texture_map(z_data.vertices, yuy_intrin, yuy_extrin);
        //std::sort(uvmap.begin(), uvmap.end(), [](float2 v1, float2 v2) {return v1.x < v2.x;});

        auto cost = calc_cost(z_data, yuy_data, uvmap);
        auto grad = calc_gradients(z_data, yuy_data, uvmap, yuy_intrin, yuy_extrin);
        return std::pair<auto_cal_algo::calib_gradients, float>();
    }

    bool auto_cal_algo::optimaize()
    {
        auto yuy_data = preprocess_yuy2_data();
        //cv::Mat res_mat(height_yuy2, width_yuy2, CV_32F, yuy_data.edges_IDT.data());

        auto ir_data = get_ir_data();

        auto z_data = preproccess_z(ir_data);
        //std::sort(z_data.weights.begin(), z_data.weights.end(), [](float v1, float v2) {return v1 < v2;});
        
        /*std::string res_filexp1("LongRange/15/binFiles/Z_edge_768x1024_single_00.bin");
        res = get_image<float>(res_filexp1, width_z, height_z);
        for (auto i = 0;i < z_data.edges.size(); i++)
        {
            auto val = z_data.edges[i];
            auto val1 = res[i];
            if (abs(val - val1) > 0.01)
                std::cout << "err";
        }*/

       

        rs2_intrinsics rgb_intrinsics = { width_yuy2, height_yuy2,
                                     959.33734, 568.44531,
                                     1355.8387,1355.1364,
                                     RS2_DISTORTION_BROWN_CONRADY,  {0.14909270, -0.49516717, -0.00067913462, 0.0010808484, 0.42839915}};


        rs2_extrinsics extrinsics = { { 0.99970001, -0.021156590, 0.012339020,
                                       0.021360161, 0.99963391, -0.016606549,
                                      -0.011983165, 0.016865131, 0.99978596 },
                                       1.1025798, 13.548738, -6.8803735 };

        /*rs2_extrinsics extrinsics = { {0.99970001, 0.021360161,  -0.011983165,
            -0.021156590, 0.99963391, 0.016865131,
            0.012339020, -0.016606549, 0.99978596},
                                      1.1025798, 13.548738, -6.8803735 };*/
       /* auto uvmap = get_texture_map(z_data.vertices, rgb_intrinsics, extrinsics);
        std::sort(uvmap.begin(), uvmap.end(), [](float2 v1, float2 v2) {return v1.x < v2.x;});*/
      
        calc_cost_and_grad(z_data, yuy_data, rgb_intrinsics, extrinsics);

       


        return true;
    }

int main()
{
    auto_cal_algo auto_cal;
    auto_cal.optimaize();


    std::cout << "Hello World!\n";

    /* std::string res_file("F9440842_scenexp1/binFiles/Z_edgeSupressed_480x640_single_00.bin");
     auto res = get_image<float>(res_file, width_z, height_z);

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


