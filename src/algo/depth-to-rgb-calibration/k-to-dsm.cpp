//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "k-to-dsm.h"
#include "debug.h"
#include "utils.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


rs2_intrinsics_double rotate_k_mat(const rs2_intrinsics_double& k_mat)
{
    rs2_intrinsics_double res = k_mat;
    res.ppx = k_mat.width - 1 - k_mat.ppx;
    res.ppy = k_mat.height - 1 - k_mat.ppy;

    return res;
}

DSM_regs librealsense::algo::depth_to_rgb_calibration::k_to_DSM::apply_ac_res_on_dsm_model
(
    const rs2_dsm_params& ac_data, 
    const DSM_regs& dsm_regs, 
    const ac_to_dsm_dir& type
)
{
    DSM_regs res;

    if (type == direct) // convert from original model to modified model
    {
        switch (ac_data.model)
        {
        case dsm_model::none: // none
            res = dsm_regs;
            break;
        case AOT: // AOT model
            res.dsm_x_scale = dsm_regs.dsm_x_scale*ac_data.h_scale;
            res.dsm_y_scale = dsm_regs.dsm_y_scale*ac_data.v_scale;
            res.dsm_x_offset = (dsm_regs.dsm_x_offset + ac_data.h_offset) / ac_data.h_scale;
            res.dsm_y_offset = (dsm_regs.dsm_y_offset + ac_data.v_offset) / ac_data.v_scale;
            break;
        case  dsm_model::TOA: // TOA model
            res.dsm_x_scale = dsm_regs.dsm_x_scale*ac_data.h_scale;
            res.dsm_y_scale = dsm_regs.dsm_y_scale*ac_data.v_scale;
            res.dsm_x_offset = (dsm_regs.dsm_x_offset + ac_data.h_offset) / dsm_regs.dsm_x_scale;
            res.dsm_y_offset = (dsm_regs.dsm_y_offset + ac_data.v_offset) / dsm_regs.dsm_y_scale;
            break;
        default:
            throw std::runtime_error(ac_data.flags[0] + "is not valid model");
            break;
        }
    }
    else if (type == inverse) // revert from modified model to original model
    {
        switch (ac_data.flags[0])
        {
            case dsm_model::none: // none
            res = dsm_regs;
            break;
                case dsm_model::AOT: // AOT model
                res.dsm_x_scale = dsm_regs.dsm_x_scale / ac_data.h_scale;
                res.dsm_y_scale = dsm_regs.dsm_y_scale / ac_data.v_scale;
                res.dsm_x_offset = dsm_regs.dsm_x_offset* ac_data.h_scale - ac_data.h_offset;
                res.dsm_y_offset = dsm_regs.dsm_y_offset* ac_data.v_scale - ac_data.v_offset;
                break;
            case dsm_model::TOA: // TOA model
                res.dsm_x_scale = dsm_regs.dsm_x_scale / ac_data.h_scale;
                res.dsm_y_scale = dsm_regs.dsm_y_scale / ac_data.v_scale;
                res.dsm_x_offset = dsm_regs.dsm_x_offset - ac_data.h_offset / res.dsm_x_scale;
                res.dsm_y_offset = dsm_regs.dsm_y_offset - ac_data.v_offset / res.dsm_y_scale;
                break;
        default:
            throw std::runtime_error(ac_data.flags[0] + "is not valid model");
            break;
        }
    }
    return res;
}

los_shift_scaling librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_ac_data_to_los_error
(
    const DSM_regs& dsm_regs, 
    const rs2_dsm_params& ac_data
)
{
    los_shift_scaling res;
    switch (ac_data.flags[0])
    {
    case dsm_model::none: // none
            res.los_scaling_x = 1;
            res.los_scaling_y = 1;
            res.los_shift_x = 0;
            res.los_shift_y = 0;
            break;
        case dsm_model::AOT:
            res.los_scaling_x = 1/ ac_data.h_scale;
            res.los_scaling_y = 1/ac_data.v_scale;
            res.los_shift_x = -ac_data.h_offset*res.los_scaling_x;
            res.los_shift_y = -ac_data.v_offset*res.los_scaling_y;
            break;
        case dsm_model::TOA:
            res.los_scaling_x = 1 / ac_data.h_scale;
            res.los_scaling_y = 1 / ac_data.v_scale;

            auto dsm_orig = apply_ac_res_on_dsm_model(ac_data, dsm_regs, inverse);

            res.los_shift_x = -ac_data.h_offset / dsm_orig.dsm_x_scale - dsm_orig.dsm_x_offset*(1 - res.los_scaling_x);
            res.los_shift_y = -ac_data.v_offset / dsm_orig.dsm_y_scale - dsm_orig.dsm_y_offset*(1 - res.los_scaling_y);
            break;
    }
    return res;
}

los_shift_scaling librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_norm_vertices_to_los
(
    const regs & regs, 
    const DSM_regs & dsm_regs, 
    const std::vector<double3>& vertices
)
{

    return los_shift_scaling();
}

pre_process_data librealsense::algo::depth_to_rgb_calibration::k_to_DSM::pre_processing
(
    const regs & regs, 
    const rs2_dsm_params & ac_data, 
    const DSM_regs & dsm_regs, 
    const rs2_intrinsics_double & k_raw, 
    const std::vector<uint8_t>& relevant_pixels_image
)
{
    pre_process_data res;
    res.relevant_pixels_image_rot = relevant_pixels_image;
    res.last_los_error = convert_ac_data_to_los_error(dsm_regs, ac_data);

    res.vertices_orig = calc_relevant_vertices(relevant_pixels_image, k_raw);
    auto dsm_res_orig = apply_ac_res_on_dsm_model(ac_data, dsm_regs, inverse);

    return res;
}

rs2_dsm_params librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_new_k_to_DSM
(
    const rs2_intrinsics_double& old_k,
    const rs2_intrinsics_double& new_k,
    const rs2_dsm_params& orig_dsm_params,
    const DSM_regs& dsm_regs,
    const regs& regs,
    const std::vector<uint8_t>& relevant_pixels_image
)
{
    auto w = old_k.width;
    auto h = old_k.height;

    auto old_k_raw = rotate_k_mat(old_k);
    auto new_k_raw = rotate_k_mat(new_k);

    auto dsm_orig = apply_ac_res_on_dsm_model(orig_dsm_params, dsm_regs, inverse);

    std::vector<uint8_t> relevant_pixels_image_rot(relevant_pixels_image.size(), 0);
    rotate_180(relevant_pixels_image.data(), relevant_pixels_image_rot.data(), w, h);
    _pre_process_data = pre_processing(regs, orig_dsm_params, dsm_regs, old_k_raw, relevant_pixels_image_rot);
    return rs2_dsm_params();
}

const pre_process_data& librealsense::algo::depth_to_rgb_calibration::k_to_DSM::get_pre_process_data() const
{
    return _pre_process_data;
}

std::vector<double3> librealsense::algo::depth_to_rgb_calibration::k_to_DSM::calc_relevant_vertices
(
    const std::vector<uint8_t>& relevant_pixels_image, 
    const rs2_intrinsics_double & k
)
{
    std::vector<double3> res;

    double k_arr[9] = { k.fx, 0, 0,
                        0, k.fy, 0,
                        k.ppx , k.ppy, 1 };

    double k_arr_inv[9] = { 0 };

    inv(k_arr, k_arr_inv);

    double k_t[9] = { 0 };

    transpose(k_arr_inv, k_t);

    for (auto i = 0; i < k.height; i++)
    {
        for (auto j = 0; j < k.width; j++)
        {
            if (relevant_pixels_image[i*k.width + j])
            {
                double3 ver;
                double3 pix = { j, i, 1 };

                ver.x = pix.x*k_t[0] + pix.y*k_t[1] + pix.z*k_t[2];
                ver.y = pix.x*k_t[3] + pix.y*k_t[4] + pix.z*k_t[5];
                ver.z = pix.x*k_t[6] + pix.y*k_t[7] + pix.z*k_t[8];

                res.push_back(ver);
            }
        }
    }
    return res;
}

los_shift_scaling librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_norm_vertices_to_los
(
    const regs & regs, 
    const DSM_regs & dsm_regs, 
    std::vector<double3> vertices
)
{
    const int angle = 45;
    los_shift_scaling res;
    auto directions = transform_to_direction(vertices);

    auto fovex_indicent_direction = directions;
    if (regs.frmw.fovex_existence_flag)
    {
        fovex_indicent_direction.resize(0);
        std::vector<double> ang_post_exp(fovex_indicent_direction.size(), 0);
        for (auto i = 0; i < ang_post_exp.size(); i++)
        {
            ang_post_exp[i] = std::acos(fovex_indicent_direction[i].z);
        }

        std::vector<double> ang_out_on_grid(angle, 0);

        for (auto i = 0; i < angle; i++)
        {
            auto fovex_nominal = regs.frmw.fovex_nominal[0] +
                2 * regs.frmw.fovex_nominal[1] +
                3 * regs.frmw.fovex_nominal[2] +
                4 * regs.frmw.fovex_nominal[3];

            ang_out_on_grid[i] = i + std::pow(i, fovex_nominal);
        }
    }
    return res;
}

std::vector<double3> librealsense::algo::depth_to_rgb_calibration::k_to_DSM::transform_to_direction(std::vector<double3> vec)
{
   std::vector<double3> res(vec.size());

   for (auto i = 0; i < vec.size(); i++)
   {
       auto norm = sqrt(vec[i].x*vec[i].x + vec[i].y*vec[i].y + vec[i].z*vec[i].z);
       res[i] = { vec[i].x / norm, vec[i].y / norm, vec[i].z / norm };
   }
   return res;
}
