//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "auto-cal-algo.h"
#include "context.h"
#include "../include/librealsense2/rsutil.h"
#include <algorithm>

#define AC_LOG_PREFIX "AC1: "
//#define AC_LOG(TYPE,MSG) LOG_##TYPE( AC_LOG_PREFIX << MSG )
#define AC_LOG(TYPE,MSG) LOG_ERROR( AC_LOG_PREFIX << MSG )
//#define AC_LOG(TYPE,MSG) std::cout << "-D- " << MSG << std::endl

namespace librealsense
{
    auto_cal_algo::auto_cal_algo(
        rs2::frame depth,
        rs2::frame ir,
        rs2::frame yuy,
        rs2::frame prev_yuy
    )
        : depth( depth )
        , ir( ir )
        , yuy( yuy )
        , prev_yuy( prev_yuy )
        , _intr( yuy.get_profile().as< rs2::video_stream_profile >().get_intrinsics() )
        , _extr( depth.get_profile().get_extrinsics_to( yuy.get_profile() ) )
        , _from( depth.get_profile().get()->profile )
        , _to( yuy.get_profile().get()->profile )
    {
    }

    bool auto_cal_algo::optimize()
    {
        AC_LOG( DEBUG, "old intr"
            << ": width: " << _intr.width
            << ", height: " << _intr.height
            << ", ppx: " << _intr.ppx
            << ", ppy: " << _intr.ppy
            << ", fx: " << _intr.fx
            << ", fy: " << _intr.fy
            << ", model: " << _intr.model
            << ", coeffs: ["
            << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2] << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4]
            << "]" );
        AC_LOG( DEBUG, "old extr:"
            << " rotation: ["
            << _extr.rotation[0] << ", " << _extr.rotation[1] << ", " << _extr.rotation[2] << ", " << _extr.rotation[3] << ", " << _extr.rotation[4] << ", "
            << _extr.rotation[5] << ", " << _extr.rotation[6] << ", " << _extr.rotation[7] << ", " << _extr.rotation[8]
            << "]  translation: ["
            << _extr.translation[0] << ", " << _extr.translation[1] << ", " << _extr.translation[2]
            << "]" );

        auto yuy_data = preprocess_yuy2_data(yuy, prev_yuy);
        auto ir_data = preprocess_ir(ir);

        auto depth_units = depth.as<rs2::depth_frame>().get_units();
        auto vf = depth.get_profile().as<rs2::video_stream_profile>();

        auto z_data = preproccess_z(depth, ir_data, vf.get_intrinsics(), depth_units);

        optimaization_params params_orig;
        params_orig.curr_calib = intrinsics_extrinsics_to_calib(_intr, _extr);

        auto optimized = false;

        auto cost = calc_cost_and_grad(z_data, yuy_data, params_orig.curr_calib);
        AC_LOG( DEBUG, "Original cost = " << cost.second );

        optimaization_params params_curr = params_orig;

        for (auto i = 0; i < _params.max_optimization_iters; i++)
        {
            auto res = calc_cost_and_grad(z_data, yuy_data, params_curr.curr_calib);

            params_curr.cost = res.second;
            params_curr.calib_gradients = res.first;

            auto new_params = back_tracking_line_search(z_data, yuy_data, params_curr);
            if ((new_params.curr_calib - params_curr.curr_calib).get_norma() < _params.min_rgb_mat_delta)
                break;

            if (abs(new_params.cost - params_curr.cost) < _params.min_cost_delta)
                break;

            params_curr = new_params;
            AC_LOG( DEBUG, "Current optimaized cost = " << params_curr.cost );

            optimized = true;
        }
        AC_LOG( DEBUG, "Optimaized cost = " << params_curr.cost );
#if 1
        auto r = params_curr.curr_calib.rot.rot;
        auto t = params_curr.curr_calib.trans;
        auto c = params_curr.curr_calib.coeffs;

        _extr = rs2_extrinsics { {r[0],r[1],r[2],r[3],r[4],r[5],r[6],r[7],r[8] },
                                {(float)t.t1, (float)t.t2, (float)t.t3} };

        _intr = rs2_intrinsics { params_curr.curr_calib.width, params_curr.curr_calib.height,  //_intr.width, _intr.height,
                                (float)params_curr.curr_calib.k_mat.ppx, (float)params_curr.curr_calib.k_mat.ppy,
                                (float)params_curr.curr_calib.k_mat.fx, (float)params_curr.curr_calib.k_mat.fy,
                                params_curr.curr_calib.model, {(float)c[0], (float)c[1], (float)c[2], (float)c[3], (float)c[4]} };

        AC_LOG( DEBUG, "new intr"
            << ": width: " << _intr.width
            << ", height: " << _intr.height
            << ", ppx: " << _intr.ppx
            << ", ppy: " << _intr.ppy
            << ", fx: " << _intr.fx
            << ", fy: " << _intr.fy
            << ", model: " << _intr.model
            << ", coeffs: ["
            << _intr.coeffs[0] << ", " << _intr.coeffs[1] << ", " << _intr.coeffs[2] << ", " << _intr.coeffs[3] << ", " << _intr.coeffs[4]
            << "]" );
        AC_LOG( DEBUG, "new extr:"
            << " rotation: ["
            << _extr.rotation[0] << ", " << _extr.rotation[1] << ", " << _extr.rotation[2] << ", " << _extr.rotation[3] << ", " << _extr.rotation[4] << ", "
            << _extr.rotation[5] << ", " << _extr.rotation[6] << ", " << _extr.rotation[7] << ", " << _extr.rotation[8]
            << "]  translation: ["
            << _extr.translation[0] << ", " << _extr.translation[1] << ", " << _extr.translation[2]
            << "]" );
#endif

        return optimized;
    }

    auto_cal_algo::z_frame_data auto_cal_algo::preproccess_z(rs2::frame depth, const ir_frame_data& ir_data, const rs2_intrinsics& depth_intrinsics, float depth_units)
    {
        z_frame_data res;

        res.frame.resize(depth.get_data_size(), 0);
        std::copy((uint16_t*)(depth.get_data()), (uint16_t*)(depth.get_data()) + depth_intrinsics.width*depth_intrinsics.height, res.frame.begin());
        res.gradient_x = calc_vertical_gradient(res.frame, depth_intrinsics.width, depth_intrinsics.height);

        res.gradient_y = calc_horizontal_gradient(res.frame, depth_intrinsics.width, depth_intrinsics.height);

        res.edges = calc_intensity(res.gradient_x, res.gradient_y);

        res.directions = get_direction(res.gradient_x, res.gradient_y);
        res.direction_deg = get_direction_deg(res.gradient_x, res.gradient_y);
        res.supressed_edges = supressed_edges(res, ir_data, depth_intrinsics.width, depth_intrinsics.height);

        auto subpixels = calc_subpixels(res, ir_data, depth_intrinsics.width, depth_intrinsics.height);
        res.subpixels_x = subpixels.first;
        res.subpixels_y = subpixels.second;

        res.closest = get_closest_edges(res, ir_data, depth_intrinsics.width, depth_intrinsics.height);

        calculate_weights(res);

        auto vertices = subedges2vertices(res, depth_intrinsics, depth_units);

        res.width = depth_intrinsics.width;
        res.height = depth_intrinsics.height;

        return res;
    }

    auto_cal_algo::yuy2_frame_data auto_cal_algo::preprocess_yuy2_data(rs2::frame color, rs2::frame prev_color)
    {
        yuy2_frame_data res;

        auto vf = color.get_profile().as<rs2::video_stream_profile>();
        std::vector<uint16_t> color_frame(color.get_data_size(), 0);
        std::copy((uint16_t*)(color.get_data()), (uint16_t*)(color.get_data()) +vf.width()*vf.height(), color_frame.begin());

        res.yuy2_frame = get_luminance_from_yuy2(color_frame);
        
        res.edges = calc_edges(res.yuy2_frame, vf.width(), vf.height());
        
        res.edges_IDT = blure_edges(res.edges, vf.width(), vf.height());
        
        res.edges_IDTx = calc_vertical_gradient(res.edges_IDT, vf.width(), vf.height());
       
        res.edges_IDTy = calc_horizontal_gradient(res.edges_IDT, vf.width(), vf.height());
       
        std::vector<uint16_t> prev_color_frame(color.get_data_size(), 0);
        res.yuy2_prev_frame = get_luminance_from_yuy2(prev_color_frame);

        res.width = vf.width();
        res.height = vf.height();

        return res;
    }

    auto_cal_algo::ir_frame_data auto_cal_algo::preprocess_ir(rs2::frame ir)
    {
        auto vf = ir.get_profile().as<rs2::video_stream_profile>();

        std::vector<uint8_t> ir_frame(ir.get_data_size(), 0);
        std::copy((uint8_t*)(ir.get_data()), (uint8_t*)(ir.get_data()) + vf.width()*vf.height(), ir_frame.begin());

        auto edges = calc_edges(ir_frame, vf.width(), vf.height());

        return { ir_frame, edges };
    }

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
        for (auto i = 0; i < ir_data.ir_edges.size(); i++)
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

        for (auto i = 0; i < gradient_x.size(); i++)
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

        for (auto i = 0; i < gradient_x.size(); i++)
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
        for (auto i = 0; i < height; i++)
        {
            for (auto j = 0; j < width; j++)
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

        for (auto i = 0; i < height; i++)
        {
            for (auto j = 0; j < width; j++)
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

        for (auto i = 0; i < height; i++)
        {
            for (auto j = 0; j < width; j++)
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
        for (auto i = 0; i < res.size(); i++)
            res[i] = yuy2[i * 2];

        return res;
    }

    std::vector<uint8_t> auto_cal_algo::get_logic_edges(std::vector<double> edges)
    {
        std::vector<uint8_t> logic_edges(edges.size(), 0);
        auto max = std::max_element(edges.begin(), edges.end());
        auto thresh = *max*_params.edge_thresh4_logic_lum;

        for (auto i = 0; i < edges.size(); i++)
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
        for (auto i = 0; i < z_data.supressed_edges.size(); i++)
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

        for (auto i = 0; i < uv.size(); i++)
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
            AC_LOG( DEBUG, "Current back tracking line search cost = " << new_params.cost );
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
        auto d_vals = biliniar_interp(yuy_data.edges_IDT, yuy_data.width, yuy_data.height, uv);
        double cost = 0;

        auto sum_of_elements = 0;
        for (auto i = 0; i < d_vals.size(); i++)
        {
            if (d_vals[i] != std::numeric_limits<double>::max())
            {
                sum_of_elements++;
                cost += d_vals[i] * z_data.weights[i];
            }
        }
        if (sum_of_elements == 0)
            return 0;
            //throw "didnt convertege";
        cost /= (double)sum_of_elements;
        return cost;
    }

    auto_cal_algo::calib auto_cal_algo::calc_gradients(const z_frame_data& z_data, const yuy2_frame_data& yuy_data, const std::vector<float2>& uv,
        const calib& curr_calib)
    {
        calib res;
        auto interp_IDT_x = biliniar_interp(yuy_data.edges_IDTx, yuy_data.width, yuy_data.height, uv);
#if 0
        std::ofstream f;
        f.open("res");
        for (auto i = 0; i < interp_IDT_x.size(); i++)
        {
            f << uv[i].x << " " << uv[i].y << std::endl;
        }
        f.close();
#endif

        auto interp_IDT_y = biliniar_interp(yuy_data.edges_IDTy, yuy_data.width, yuy_data.height, uv);

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

    std::pair< std::vector<float2>, std::vector<double>> auto_cal_algo::calc_rc(const z_frame_data & z_data, const yuy2_frame_data & yuy_data, const calib& curr_calib)
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

        exp1 = rc + 2 * (double)d[3] * x1 + 6 * (double)d[2] * y1 + y1 * (2 * (double)d[0] * y1 + 4 * (double)d[1] * y1*xy2 + 6 * (double)d[4] * y1*x2_y2);
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

        auto exp1 = z * (cos_a*cos_b) +
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

        auto exp1 = z * (cos_a*cos_b) + x * ((sin_a*sin_g - cos_a * cos_g*sin_b))
            + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2]);

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

        auto res = (((y*(+(sin_a*sin_g - cos_a * cos_g*sin_b)) - x * ((cos_g*sin_a + cos_a * sin_b*sin_g)))
            *(z*(ppy * cos_a*cos_b - fy * cos_b*sin_a) + x * (fy*(cos_a*sin_g + cos_g * sin_a*sin_b)
                + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g)
                    + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g))
                + (fy * t[1] + ppy * t[2])) - (y*(fy*(cos_a*sin_g + cos_g * sin_a*sin_b) + ppy * (sin_a*sin_g - cos_a * cos_g*sin_b))
                    - x * (fy*(cos_a*cos_g - sin_a * sin_b*sin_g) + ppy * (cos_g*sin_a + cos_a * sin_b*sin_g)))*(z*(cos_a*cos_b)
                        + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * (+(cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))
            *(rc + 2 * d[3] * x1 + 6 * d[2] * y1 + y1 * (2 * d[0] * y1 + 4 * d[1] * y1*(xy2)+6 * d[4] * y1*x2_y2)))
            / (exp1*exp1) + (fy*((y*(+(sin_a*sin_g - cos_a * cos_g*sin_b)) - x * (+(cos_g*sin_a + cos_a * sin_b*sin_g)))
                *(z*(fx*sin_b + ppx * cos_a*cos_b) + x * (ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g)
                    + y * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g) + (fx*t[0] + ppx * t[2]))
                - (y*(ppx * (sin_a*sin_g - cos_a * cos_g*sin_b) + fx * cos_b*cos_g) - x
                    * (+ppx * (cos_g*sin_a + cos_a * sin_b*sin_g) - fx * cos_b*sin_g))*(z*(cos_a*cos_b)
                        + x * ((sin_a*sin_g - cos_a * cos_g*sin_b)) + y * ((cos_g*sin_a + cos_a * sin_b*sin_g)) + (t[2])))
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

#if 0
        std::ofstream f;
        f.open("uvmap");
        for (auto i = 0; i < uvmap.size(); i++)
        {
            f << uvmap[i].x << " " << uvmap[i].y << std::endl;
        }
        f.close();
#endif
        auto cost = calc_cost(z_data, yuy_data, uvmap);
        auto grad = calc_gradients(z_data, yuy_data, uvmap, curr_calib);
        return { grad, cost };
    }

    auto_cal_algo::params::params()
    {
        normelize_mat.k_mat = { 0.354172020000000, 0.265703050000000,1.001765500000000, 1.006649100000000 };
        normelize_mat.rot_angles = { 1508.94780000000, 1604.94300000000,649.384340000000 };
        normelize_mat.trans = { 0.913008390000000, 0.916982890000000, 0.433054570000000 };
    }

    void auto_cal_algo::calib::copy_coefs(calib & obj)
    {
        obj.width = this->width;
        obj.height = this->height;

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

    std::pair< rs2_intrinsics, rs2_extrinsics> auto_cal_algo::convert_calib_to_intrinsics_extrinsics(auto_cal_algo::calib curr_calib)
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

    auto_cal_algo::calib auto_cal_algo::intrinsics_extrinsics_to_calib(rs2_intrinsics intrin, rs2_extrinsics extrin)
    {
        auto_cal_algo::calib cal;
        auto rot = extrin.rotation;
        auto trans = extrin.translation;
        auto coeffs = intrin.coeffs;

        cal.height = intrin.height;
        cal.width = intrin.width;
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
} // namespace librealsense
