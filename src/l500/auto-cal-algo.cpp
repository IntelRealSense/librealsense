//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "auto-cal-algo.h"
#include "../include/librealsense2/rsutil.h"
#include "proc/pointcloud.h"

namespace librealsense
{

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

    std::vector<float> auto_cal_algo::calc_intensity(std::vector<float> image1, std::vector<float> image2)
    {
        std::vector<float> res(image1.size(), 0);
        //check that sizes are equal
        for (auto i = 0; i < image1.size(); i++)
        {
            res[i] = sqrt(pow(image1[i] / 8.f, 2) + pow(image2[i] / 8.f, 2));
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

    auto_cal_algo::z_frame_data auto_cal_algo::preproccess_z(rs2::frame depth, const ir_frame_data& ir_data)
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

    auto_cal_algo::yuy2_frame_data auto_cal_algo::preprocess_yuy2_data(rs2::frame yuy)
    {
        yuy2_frame_data res;

        auto yuy2_data = get_yuy2_image(width_yuy2, height_yuy2);
        res.yuy2_frame = get_luminance_from_yuy2(yuy2_data);

        res.edges = calc_gradients(res.yuy2_frame, width_yuy2, height_yuy2);

        res.edges_IDT = blure_edges(res.edges, width_yuy2, height_yuy2, gamma, alpha);

        res.edges_IDTx = calc_vertical_gradient(res.edges_IDT, width_yuy2, height_yuy2);

        res.edges_IDTy = calc_horizontal_gradient(res.edges_IDT, width_yuy2, height_yuy2);

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

    std::vector<rs2_vertex> auto_cal_algo::subedges2vertices(z_frame_data z_data, const rs2_intrinsics& intrin, float depth_units)
    {
        std::vector<rs2_vertex> res(z_data.subpixels_x.size());
        deproject_sub_pixel(res, intrin, z_data.subpixels_x.data(), z_data.subpixels_y.data(), z_data.closest.data(), depth_units);
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
        const std::vector<rs2_vertex> points,
        const rs2_intrinsics &intrinsics,
        const rs2_extrinsics& extr)
    {
        std::vector<float2> uv(points.size());

        for (auto i = 0; i < points.size(); ++i)
        {
            float3 p = {}; 
            rs2_transform_point_to_point(&p.x, &extr, &points[i].xyz[0]); 
            
                 //auto tex_xy = project_to_texcoord(&mapped_intr, trans);
                 // Store intermediate results for poincloud filters
            float2 pixel = {}; 
            rs2_project_point_to_pixel(&pixel.x, &intrinsics, &p.x); 
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

            if (x1 < 0 || x1 > width || x2< 0 || x2 >= width ||
                y1 < 0 || y1 > height || y2< 0 || y2 >= height)
            {
                res[i] = NAN;
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

    bool auto_cal_algo::optimaize(rs2::frame depth, rs2::frame ir, rs2::frame yuy, rs2::frame prev_yuy, const calibration & old_calib, calibration * new_calib)
    {
        std::vector<float> arr = { 1,0,0,0 };
        std::vector<float2> p = { { -0.5,-0.5} };
        auto interp = biliniar_interp(arr, 2, 2, p);

        auto ir_data = get_ir_data();

        auto z_data = preproccess_z(depth, ir_data);
        std::sort(z_data.weights.begin(), z_data.weights.end(), [](float v1, float v2) {return v1 < v2;});
        auto yuy_data = preprocess_yuy2_data(yuy);

        /*std::string res_file1("LongRange/15/binFiles/Z_edge_768x1024_single_00.bin");
        res = get_image<float>(res_file1, width_z, height_z);
        for (auto i = 0;i < z_data.edges.size(); i++)
        {
            auto val = z_data.edges[i];
            auto val1 = res[i];
            if (abs(val - val1) > 0.01)
                std::cout << "err";
        }*/

        rs2_intrinsics depth_intrin = { width_z, height_z, 
             529.27344, 402.32031,
             731.27344,731.97656,
            RS2_DISTORTION_INVERSE_BROWN_CONRADY, {0, 0, 0, 0, 0} };

        float depth_units = 0.25f;
        auto vertices = subedges2vertices(z_data, depth_intrin, depth_units);
        std::sort(vertices.begin(), vertices.end(), [](rs2_vertex v1, rs2_vertex v2) {return v1.xyz[0] < v2.xyz[0];});

        rs2_intrinsics rgb_intrinsics = { width_yuy2, height_yuy2,
                                     959.33734, 568.44531,
                                     1355.8387,1355.1364,
                                     RS2_DISTORTION_NONE, {1,1,1,1,1},/* {0.14909270, -0.49516717, -0.00067913462, 0.0010808484, 0.42839915}*/ };


       /* rs2_extrinsics extrinsics = { { 0.99970001, 0.021360161, -0.011983165,
                                        -0.021156590, 0.99963391, 0.016865131,
                                        0.012339020, -0.016606549, 0.99978596 },
                                       -0.011983165, 13.260437, -6.7013960 };*/

       
        rs2_extrinsics extrinsics = { { 0.99970001, -0.021156590, 0.012339020,
                                       0.021360161, 0.99963391, -0.016606549,
                                      -0.011983165, 0.016865131, 0.99978596 },
                                       1.1025798, 13.548738, -6.8803735 };
        
            
            
        /*vertices[0].xyz[0] = 1;
        vertices[0].xyz[1] = 1;
        vertices[0].xyz[2] = 1;*/

        auto res = get_texture_map(vertices, rgb_intrinsics, extrinsics);
        std::sort(res.begin(), res.end(), [](float2 v1, float2 v2) {return v1.x < v2.x;});

       
        auto interp_IDT = biliniar_interp(yuy_data.edges_IDT, width_yuy2, height_yuy2, res);
        std::sort(interp_IDT.begin(), interp_IDT.end());
        return true;
    }
} // namespace librealsense
