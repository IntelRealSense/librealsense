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
      
        std::string z_file = "LongRange/15/Z_GrayScale_1024x768_0001.raw";//"LongRange/13/Z_GrayScale_1024x768_00.01.21.3573_F9440687_0001.raw";
        std::string i_file = "LongRange/15/I_GrayScale_1024x768_0001.raw";//"LongRange/13/I_GrayScale_1024x768_00.01.21.3573_F9440687_0000.raw";
        std::string yuy2_file = "LongRange/15/YUY2_YUY2_1920x1080_0001.raw";
        std::string yuy2_prev_file = "LongRange/15/YUY2_YUY2_1920x1080_0001.raw";

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

        enum convolution_operation
        {
            convolution_dot_product,
            convolution_max_element
        };

        std::map < direction, std::pair<int, int>> dir_map = { {deg_0, {1, 0}},
                                                             {deg_45, {1, 1}},
                                                             {deg_90, {0, 1}},
                                                             {deg_135, {-1, 1}} };



        bool optimaize(rs2::frame depth, rs2::frame ir, rs2::frame yuy, rs2::frame prev_yuy, const calibration& old_calib, calibration* new_calib);

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

        z_frame_data preproccess_z(rs2::frame depth, const ir_frame_data & ir_data);

        std::vector<float> blure_edges(std::vector<float> edges, uint32_t image_widht, uint32_t image_height, float gamma, float alpha);

        std::vector<uint8_t> get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh);

        yuy2_frame_data preprocess_yuy2_data(rs2::frame yuy);

        ir_frame_data get_ir_data();

        std::vector<uint8_t> get_logic_edges(std::vector<float> edges);

        bool is_movement_in_images(const yuy2_frame_data & yuy);

        bool is_scene_valid(yuy2_frame_data yuy);

        std::vector<float> calculate_weights(std::vector<float> edges);

        std::vector <rs2_vertex> subedges2vertices(z_frame_data z_data, const rs2_intrinsics& intrin, float depth_units);

        template<class T>
        std::vector<float> calc_gradients(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            auto vertical_edge = calc_vertical_gradient(image, image_widht, image_height);

            auto horizontal_edge = calc_horizontal_gradient(image, image_widht, image_height);

            auto edges = calc_intensity(vertical_edge, horizontal_edge);

            return edges;
        }

        template<class T>
        float dot_product(std::vector<T> sub_image, std::vector<float> mask, bool divide = false)
        {
            //check that sizes are equal
            float res = 0;

            for (auto i = 0; i < sub_image.size(); i++)
            {
                res += sub_image[i] * mask[i];
            }
            if (divide)
                res /= mask.size();
            return res;
        }

        template<class T>
        std::vector<float> convolution(std::vector<T> image, std::vector<float> mask, uint32_t image_widht, uint32_t image_height, uint32_t mask_widht, uint32_t mask_height, convolution_operation op = convolution_dot_product, bool divide = false)
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

                    float res1 = 0;
                    if (op == convolution_dot_product)
                    {
                        res1 = dot_product(sub_image, mask);
                        if (divide)
                            res1 /= mask.size();
                    }
                    else if (op == convolution_max_element)
                    {
                        auto max = *std::max_element(sub_image.begin(), sub_image.end());
                        res1 = (float)max;
                    }
                    auto mid = (i + mask_height / 2) * image_widht + j + mask_widht / 2;
                    res[mid] = res1;
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

            return convolution(image, vertical_gradients, image_widht, image_height, 3, 3);
        }


        template<class T>
        std::vector<float> calc_horizontal_gradient(std::vector<T> image, uint32_t image_widht, uint32_t image_height)
        {
            std::vector<float> horizontal_gradients = { -1, -2, -1,
                                                          0,  0,  0,
                                                          1,  2,  1 };

            return convolution(image, horizontal_gradients, image_widht, image_height, 3, 3);
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
} // namespace librealsense
