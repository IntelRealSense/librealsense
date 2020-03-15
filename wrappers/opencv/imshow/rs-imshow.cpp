// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <opencv2/opencv.hpp>   // Include OpenCV API
#include <librealsense2/hpp/rs_internal.hpp>
#include <iostream>
//C:\Users\aangerma\source\repos\rgb2depth\rgb2depth\opencv - master\modules\gapi\include\opencv2\gapi\own
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

std::string z_file = "LongRange/15/Z_GrayScale_1024x768_0001.raw";//"LongRange/13/Z_GrayScale_1024x768_00.01.21.3573_F9440687_0001.raw";
std::string i_file = "LongRange/15/I_GrayScale_1024x768_0001.raw";//"LongRange/13/I_GrayScale_1024x768_00.01.21.3573_F9440687_0000.raw";
std::string yuy2_file = "LongRange/15/YUY2_YUY2_1920x1080_0000.raw";
std::string yuy2_prev_file = "LongRange/15/YUY2_YUY2_1920x1080_0001.raw";

//std::string z_file = "F9440842_scene1/ZIRGB/Z_GrayScale_640x480_00.03.23.3354_F9440842_0000.bin";
//std::string i_file = "F9440842_scene1/ZIRGB/I_GrayScale_640x480_00.03.23.3354_F9440842_0000.bin";
//std::string yuy2_file("F9440842_scene1/ZIRGB/YUY2_YUY2_1920x1080_00.03.23.2369_F9440842_0000.bin");

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

std::map < direction, std::pair<int, int>> dir_map = { {deg_0, {1, 0}},
                                                         {deg_45, {1, 1}},
                                                         {deg_90, {0, 1}},
                                                         {deg_135, {-1, 1}} };

enum convolution_operation
{
    convolution_dot_product,
    convolution_max_element
};
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

std::vector<float> calc_intensity(std::vector<float> image1, std::vector<float> image2)
{
    std::vector<float> res(image1.size(), 0);
    //check that sizes are equal
    for (auto i = 0; i < image1.size(); i++)
    {
        res[i] = sqrt(pow(image1[i] / 8.f, 2) + pow(image2[i] / 8.f, 2));
    }
    return res;
}

template<class T>
std::vector<float> convolution(std::vector<T> image, std::vector<float> mask, uint image_widht, uint image_height, uint mask_widht, uint mask_height, convolution_operation op = convolution_dot_product, bool divide = false)
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
std::vector<float> calc_vertical_gradient(std::vector<T> image, uint image_widht, uint image_height)
{
    std::vector<float> vertical_gradients = { -1, 0, 1,
                                               -2, 0, 2,
                                               -1, 0, 1 };

    return convolution(image, vertical_gradients, image_widht, image_height, 3, 3);
}


template<class T>
std::vector<float> calc_horizontal_gradient(std::vector<T> image, uint image_widht, uint image_height)
{
    std::vector<float> horizontal_gradients = { -1, -2, -1,
                                                  0,  0,  0,
                                                  1,  2,  1 };

    return convolution(image, horizontal_gradients, image_widht, image_height, 3, 3);
}

template<class T>
std::vector<float> calc_gradients(std::vector<T> image, uint image_widht, uint image_height)
{
    auto vertical_edge = calc_vertical_gradient(image, image_widht, image_height);

    auto horizontal_edge = calc_horizontal_gradient(image, image_widht, image_height);

    auto edges = calc_intensity(vertical_edge, horizontal_edge);

    return edges;
}

std::vector<uint8_t> get_luminance_from_yuy2(std::vector<uint16_t> yuy2_imagh)
{
    std::vector<uint8_t> res(yuy2_imagh.size(), 0);
    auto yuy2 = (uint8_t*)yuy2_imagh.data();
    for (auto i = 0;i < res.size(); i++)
        res[i] = yuy2[i * 2];

    return res;
}

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

std::vector<float> blure_edges(std::vector<float> edges, uint image_widht, uint image_height, float gamma, float alpha)
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
    auto data = get_image<uint16_t>(z_file, width, height);
    cv::Mat mat(height, width, CV_16U, (char*)data.data());
    return data;
}

std::vector<uint8_t> get_ir_image(uint32_t width, uint32_t height)
{
    auto data = get_image<uint8_t>(i_file, width, height);
    cv::Mat mat(height, width, CV_8U, (char*)data.data());
    return data;
}

std::vector<uint16_t> get_yuy2_image(uint32_t width, uint32_t height)
{
    auto data = get_image<uint16_t>(yuy2_file, width, height);
    cv::Mat mat(height, width, CV_16U, (char*)data.data());
    return data;
}
std::vector<uint16_t> get_yuy2_prev_image(uint32_t width, uint32_t height)
{
    auto data = get_image<uint16_t>(yuy2_prev_file, width, height);
    cv::Mat mat(height, width, CV_16U, (char*)data.data());
    return data;
}


std::vector< direction> get_direction(std::vector<float> gradient_x, std::vector<float> gradient_y)
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

std::vector< float> get_direction_deg(std::vector<float> gradient_x, std::vector<float> gradient_y)
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

std::pair< uint32_t, uint32_t> get_prev_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height)
{

    auto d = dir_map[dir];

    auto edge_minus_idx = j - d.first;
    auto edge_minus_idy = i - d.second;


    edge_minus_idx = edge_minus_idx < 0 ? 0 : edge_minus_idx;
    edge_minus_idy = edge_minus_idy < 0 ? 0 : edge_minus_idy;

    return { edge_minus_idx, edge_minus_idy };
}

std::pair< uint32_t, uint32_t> get_next_index(direction dir, uint32_t i, uint32_t j, uint32_t width, uint32_t height)
{
    auto d = dir_map[dir];

    auto edge_plus_idx = j + d.first;
    auto edge_plus_idy = i + d.second;

    edge_plus_idx = edge_plus_idx < 0 ? 0 : edge_plus_idx;
    edge_plus_idy = edge_plus_idy < 0 ? 0 : edge_plus_idy;

    return { edge_plus_idx, edge_plus_idy };
}

void zero_invalid_edges(z_frame_data& z_data, ir_frame_data ir_data)
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

std::vector< float> supressed_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
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

std::vector<uint16_t > get_closest_edges(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
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

std::pair<std::vector< float>, std::vector< float>> calc_subpixels(z_frame_data& z_data, ir_frame_data ir_data, uint32_t width, uint32_t height)
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



yuy2_frame_data get_yuy2_data(uint32_t width, uint32_t height)
{
    yuy2_frame_data res;

    auto yuy2_data = get_yuy2_image(width, height);
    res.yuy2_frame = get_luminance_from_yuy2(yuy2_data);
    cv::Mat lum1(height, width, CV_8U, (char*)res.yuy2_frame.data());

    res.edges = calc_gradients(res.yuy2_frame, width, height);
    cv::Mat yuy_edges_mat(height, width, CV_32F, res.edges.data());

    res.edges_IDT = blure_edges(res.edges, width, height, gamma, alpha);
    cv::Mat blured_mat_x(height, width, CV_32F, res.edges_IDT.data());

    res.edges_IDTx = calc_vertical_gradient(res.edges_IDT, width, height);
    cv::Mat blured_mat_y(height, width, CV_32F, res.edges_IDTx.data());

    res.edges_IDTy = calc_horizontal_gradient(res.edges_IDT, width, height);
    cv::Mat blured_mat(height, width, CV_32F, res.edges_IDTy.data());

    auto yuy2_data_prev = get_yuy2_prev_image(width, height);
    res.yuy2_prev_frame = get_luminance_from_yuy2(yuy2_data_prev);
    cv::Mat lum1_prev(height, width, CV_8U, (char*)res.yuy2_prev_frame.data());


    return res;
}

ir_frame_data get_ir_data(uint32_t width, uint32_t height)
{
    auto data = get_ir_image(width, height);
    cv::Mat data_ir_mat(height, width, CV_8S, data.data());
    auto edges = calc_gradients(data, width, height);
    cv::Mat ir_edges_mat(height, width, CV_32F, edges.data());

    return { data, edges };
}

z_frame_data get_z_data(const ir_frame_data& ir_data, uint32_t width, uint32_t height)
{
    z_frame_data res;
    res.frame = get_depth_image(width, height);
    cv::Mat data_z_mat(height, width, CV_16S, res.frame.data());

    res.gradient_x = calc_vertical_gradient(res.frame, width, height);

    res.gradient_y = calc_horizontal_gradient(res.frame, width, height);

    res.edges = calc_intensity(res.gradient_x, res.gradient_y);
    cv::Mat z_edges_mat(height, width, CV_32F, res.edges.data());

    res.directions = get_direction(res.gradient_x, res.gradient_y);
    res.direction_deg = get_direction_deg(res.gradient_x, res.gradient_y);
    cv::Mat z_directions_mat(height, width, CV_32S, res.directions.data());

    res.supressed_edges = supressed_edges(res, ir_data, width, height);

    auto subpixels = calc_subpixels(res, ir_data, width, height);

    /*res.subpixels_x = subpixels.first;
    res.subpixels_y = subpixels.second;*/

    res.closest = get_closest_edges(res, ir_data, width, height);
    //zero_invalid_edges(res, ir_data);

    return res;
}
std::vector<uint8_t> get_logic_edges(std::vector<float> edges)
{
    std::vector<uint8_t> logic_edges(edges.size(), 0);
    cv::Mat edges_mat(height_yuy2, width_yuy2, CV_8U, edges.data());
    auto max = std::max_element(edges.begin(), edges.end());
    auto thresh = *max*edge_thresh4_logic_lum;

    for (auto i = 0;i < edges.size(); i++)
    {
        if (abs(edges[i]) > thresh)
        {
            logic_edges[i] = 1;
        }
        //logic_edges[i] = abs(edges[i]) > thresh ? 1 : 0;
    }
    cv::Mat ir_edges_mat(height_yuy2, width_yuy2, CV_8U, logic_edges.data());
    return logic_edges;
}

//TODO:handle the edges
std::vector<float> imdilate(std::vector<uint8_t> edges)
{
    std::vector<float> vertical_gradients = { 1, 1, 1,
                                                1, 1, 1,
                                                1, 1, 1 };

    return convolution(edges, vertical_gradients, width_yuy2, height_yuy2, 3, 3, convolution_max_element, true);
}

template<class T>
std::vector<T> subtruct_images(std::vector<T> image1, std::vector<T> image2)
{
    std::vector<T> res(image1.size());
    for (auto i = 0;i < image1.size();i++)
    {
        res[i] = image1[i] - image2[i];
    }

    return res;

}
bool is_movement_in_images(const yuy2_frame_data& yuy)
{
    auto prev_frame_edges = calc_gradients(yuy.yuy2_prev_frame, width_yuy2, height_yuy2);
    cv::Mat prev_frame_edges_mat(height_yuy2, width_yuy2, CV_32F, prev_frame_edges.data());

    auto logic_edges = get_logic_edges(prev_frame_edges);
    cv::Mat data_z_mat(height_yuy2, width_yuy2, CV_8U, logic_edges.data());

    auto dilate = imdilate(logic_edges);
    cv::Mat dilate_mat(height_yuy2, width_yuy2, CV_32F, dilate.data());

    auto diff_img = subtruct_images(yuy.yuy2_prev_frame, yuy.yuy2_frame);

    std::vector<float> gauss = { 0.00296901674395050,0.0133062098910137,0.0219382312797146,0.0133062098910137,0.00296901674395050,
        0.0133062098910137,0.0596342954361801,0.0983203313488458,0.0596342954361801,0.0133062098910137,
        0.0219382312797146,0.0983203313488458,0.162102821637127,0.0983203313488458,0.0219382312797146,
        0.0133062098910137,0.0596342954361801,0.0983203313488458,0.0596342954361801,0.0133062098910137,
        0.00296901674395050,0.0133062098910137,0.0219382312797146,0.0133062098910137,0.00296901674395050 };



    auto res = convolution(diff_img, gauss, width_yuy2, height_yuy2, 5, 5, convolution_dot_product, true);
    cv::Mat res_mat(height_yuy2, width_yuy2, CV_32F, res.data());

    return true;
}

bool is_scene_valid(yuy2_frame_data yuy)
{
    return !is_movement_in_images(yuy);
}

std::vector<float> calculate_weights(z_frame_data z_data)
{
    std::vector<float> res;
    for (auto i = 0;i < z_data.supressed_edges.size();i++)
    {
        if (z_data.supressed_edges[i])
            res.push_back(std::min(std::max(z_data.supressed_edges[i] - grad_z_min, 0.f), grad_z_max - grad_z_min));
    }

    return res;
}

std::vector<rs2_vertex> get_points(std::vector<rs2_vertex> points, rs2_intrinsics intrinsics, float depth_units)
{
    std::vector<rs2_vertex> vertices(points.size());

    float* point = &points[0].xyz[0];
    for (auto i = 0;i < points.size();i++)
    {
        const float pixel[] = { points[i].xyz[0],points[i].xyz[1] };

        rs2_deproject_pixel_to_point(point, &intrinsics, pixel, points[i].xyz[2] * depth_units);
        point += 3;
    }
    return points;
}

std::vector<rs2_vertex> get_points(std::vector<float> x, std::vector<float> y, std::vector<uint16_t> z, rs2_intrinsics intrinsics, float depth_units)
{
    std::vector<rs2_vertex> points(x.size());

    float* point = &points[0].xyz[0];
    for (auto i = 0;i < x.size();i++)
    {
        const float pixel[] = { x[i], y[i] };
        point[0] = x[i];
        point[1] = y[i];
        point[2] = z[i];
        //rs2_deproject_pixel_to_point(point, &intrinsics, pixel, z[i]*depth_units);
        point += 3;
    }
    return points;
}

int main()
{
    std::string res_file("LongRange/15/binFiles/weights_1977x1_single_00.bin");
    auto res = get_image<float>(res_file, width_z, height_z);

    auto yuy2_data = get_yuy2_data(width_yuy2, height_yuy2);
    auto scene_valid = is_scene_valid(yuy2_data);

    auto ir_data = get_ir_data(width_z, height_z);
    cv::Mat ir_edges_mat(height_z, width_z, CV_32F, ir_data.ir_edges.data());


    auto z_data = get_z_data(ir_data, width_z, height_z);
    cv::Mat z_edges_mat(height_z, width_z, CV_32F, z_data.edges.data());

    z_data.weights = calculate_weights(z_data);
    cv::Mat supressed_edges_mat(height_z, width_z, CV_32F, z_data.subpixels_x.data());

    rs2_intrinsics depth_intrinsics = { width_yuy2, height_yuy2,
        529.27344,402.32031,
         731.27344,731.97656,
         RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

    //std::sort(z_data.subpixels_x.begin(), z_data.subpixels_x.end());
    auto points = get_points(z_data.subpixels_x, z_data.subpixels_y, z_data.closest, depth_intrinsics, 0.25f);
    std::sort(points.begin(), points.end(), [](rs2_vertex v1, rs2_vertex v2) {return v1.xyz[0] < v2.xyz[0];});
    points = get_points(points, depth_intrinsics, 0.25f);
    cv::Mat res_mat(height_z, width_z, CV_32F, res.data());
    //todo check y
    for (auto i = 0;i < z_data.weights.size(); i++)
    {
        auto val = z_data.weights[i];
        auto val1 = res[i];
        if (abs(val - val1) > 0.01)
            std::cout << "err";
    }



    rs2_intrinsics color_intrinsics = { width_yuy2, height_yuy2,
          1,1,
          0,0,
          RS2_DISTORTION_BROWN_CONRADY ,{ 0,0,0,0,0 } };

    /*1367.2693	13.029652	942.88477 - 5105.6787
        - 21.656006	1345.2003	591.17822	14449.271
        0.012339020 - 0.016606549	0.99978596 - 6.8803735*/
        /* auto color_stream = color_sensor.add_video_stream({ RS2_STREAM_COLOR, 0, 1,
             width_yuy2, height_yuy2, 60, 2,
                                     RS2_FORMAT_UYVY, color_intrinsics });*/
    rs2::pointcloud pc;


    //is_movement_in_images(yuy2_data);





    std::cout << "Hello World!\n";

    /* std::string res_file("F9440842_scene1/binFiles/Z_edgeSupressed_480x640_single_00.bin");
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


