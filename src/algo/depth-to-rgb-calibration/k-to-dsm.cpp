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

librealsense::algo::depth_to_rgb_calibration::k_to_DSM::k_to_DSM
(
    const rs2_dsm_params & orig_dsm_params, 
    algo::depth_to_rgb_calibration::algo_calibration_info const & cal_info, 
    algo::depth_to_rgb_calibration::algo_calibration_registers const & cal_regs)
    :_orig_dsm_params(orig_dsm_params),
    _cal_info(cal_info),
    _cal_regs(cal_regs)
{
}

algo_calibration_registers librealsense::algo::depth_to_rgb_calibration::k_to_DSM::apply_ac_res_on_dsm_model
(
    const rs2_dsm_params& ac_data, 
    const algo_calibration_registers& dsm_regs,
    const ac_to_dsm_dir& type
)
{
    algo_calibration_registers res;

    if (type == direct) // convert from original model to modified model
    {
        switch (ac_data.model)
        {
        case dsm_model::none: // none
            res = dsm_regs;
            break;
        case AOT: // AOT model
            res.EXTLdsmXscale = dsm_regs.EXTLdsmXscale*ac_data.h_scale;
            res.EXTLdsmYscale = dsm_regs.EXTLdsmYscale*ac_data.v_scale;
            res.EXTLdsmXoffset = (dsm_regs.EXTLdsmXoffset + ac_data.h_offset) / ac_data.h_scale;
            res.EXTLdsmYoffset = (dsm_regs.EXTLdsmYoffset + ac_data.v_offset) / ac_data.v_scale;
            break;
        case  dsm_model::TOA: // TOA model
            res.EXTLdsmXscale = dsm_regs.EXTLdsmXscale*ac_data.h_scale;
            res.EXTLdsmYscale = dsm_regs.EXTLdsmYscale*ac_data.v_scale;
            res.EXTLdsmXoffset = (dsm_regs.EXTLdsmXoffset + ac_data.h_offset) / dsm_regs.EXTLdsmXscale;
            res.EXTLdsmYoffset = (dsm_regs.EXTLdsmYoffset + ac_data.v_offset) / dsm_regs.EXTLdsmYscale;
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
                res.EXTLdsmXscale = dsm_regs.EXTLdsmXscale / ac_data.h_scale;
                res.EXTLdsmYscale = dsm_regs.EXTLdsmYscale / ac_data.v_scale;
                res.EXTLdsmXoffset = dsm_regs.EXTLdsmXoffset* ac_data.h_scale - ac_data.h_offset;
                res.EXTLdsmYoffset = dsm_regs.EXTLdsmYoffset* ac_data.v_scale - ac_data.v_offset;
                break;
            case dsm_model::TOA: // TOA model
                res.EXTLdsmXscale = dsm_regs.EXTLdsmXscale / ac_data.h_scale;
                res.EXTLdsmYscale = dsm_regs.EXTLdsmYscale / ac_data.v_scale;
                res.EXTLdsmXoffset = dsm_regs.EXTLdsmXoffset - ac_data.h_offset / res.EXTLdsmXscale;
                res.EXTLdsmYoffset = dsm_regs.EXTLdsmYoffset - ac_data.v_offset / res.EXTLdsmYscale;
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
    const algo_calibration_registers& algo_calibration_registers,
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

            auto dsm_orig = apply_ac_res_on_dsm_model(ac_data, algo_calibration_registers, inverse);

            res.los_shift_x = -ac_data.h_offset / dsm_orig.EXTLdsmXscale - dsm_orig.EXTLdsmXoffset*(1 - res.los_scaling_x);
            res.los_shift_y = -ac_data.v_offset / dsm_orig.EXTLdsmYscale - dsm_orig.EXTLdsmYoffset*(1 - res.los_scaling_y);
            break;
    }
    return res;
}

pre_process_data librealsense::algo::depth_to_rgb_calibration::k_to_DSM::pre_processing
(
    const algo_calibration_info& regs,
    const rs2_dsm_params& ac_data,
    const algo_calibration_registers& algo_calibration_registers,
    const rs2_intrinsics_double& k_raw,
    const std::vector<uint8_t>& relevant_pixels_image
)
{
    pre_process_data res;
    res.relevant_pixels_image_rot = relevant_pixels_image;
    res.last_los_error = convert_ac_data_to_los_error(algo_calibration_registers, ac_data);

    res.vertices_orig = calc_relevant_vertices(relevant_pixels_image, k_raw);
    auto dsm_res_orig = apply_ac_res_on_dsm_model(ac_data, algo_calibration_registers, inverse);
    res.los_orig = convert_norm_vertices_to_los(regs, dsm_res_orig, res.vertices_orig);
    return res;
}

rs2_dsm_params librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_new_k_to_DSM
(
    const rs2_intrinsics_double& old_k,
    const rs2_intrinsics_double& new_k,
    const std::vector<uint8_t>& relevant_pixels_image
)
{
    auto w = old_k.width;
    auto h = old_k.height;

    auto old_k_raw = rotate_k_mat(old_k);
    auto new_k_raw = rotate_k_mat(new_k);

    auto dsm_orig = apply_ac_res_on_dsm_model(_orig_dsm_params, _cal_regs, inverse);

    std::vector<uint8_t> relevant_pixels_image_rot(relevant_pixels_image.size(), 0);
    rotate_180(relevant_pixels_image.data(), relevant_pixels_image_rot.data(), w, h);
    _pre_process_data = pre_processing(_cal_info, _orig_dsm_params, _cal_regs, old_k_raw, relevant_pixels_image_rot);

    //convert_k_to_los_error
    return rs2_dsm_params();
}

const pre_process_data& librealsense::algo::depth_to_rgb_calibration::k_to_DSM::get_pre_process_data() const
{
    return _pre_process_data;
}

los_shift_scaling librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_k_to_los_error(pre_process_data const & pre_process_data, rs2_intrinsics_double const & k_raw)
{
    return los_shift_scaling();
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
                double3 pix = { (double)j, (double)i, (double)1 };

                ver.x = pix.x*k_t[0] + pix.y*k_t[1] + pix.z*k_t[2];
                ver.y = pix.x*k_t[3] + pix.y*k_t[4] + pix.z*k_t[5];
                ver.z = pix.x*k_t[6] + pix.y*k_t[7] + pix.z*k_t[8];

                res.push_back(ver);
            }
        }
    }
    return res;
}

std::vector<double2> librealsense::algo::depth_to_rgb_calibration::k_to_DSM::convert_norm_vertices_to_los
(
    const algo_calibration_info& regs,
    const algo_calibration_registers& algo_calibration_registers,
    std::vector<double3> vertices
)
{
    const int angle = 45;
    auto directions = transform_to_direction(vertices);

    auto fovex_indicent_direction = directions;
    if (regs.FRMWfovexExistenceFlag)
    {
        std::fill(fovex_indicent_direction.begin(), fovex_indicent_direction.end(), double3{ 0,0,0 });
        std::vector<double> ang_post_exp(fovex_indicent_direction.size(), 0);
        for (auto i = 0; i < ang_post_exp.size(); i++)
        {
            ang_post_exp[i] = std::acos(directions[i].z)* 180.0 / PI;
        }

        std::vector<double> ang_grid(angle, 0);
        std::vector<double> ang_out_on_grid(angle, 0);

        for (auto i = 0; i < angle; i++)
        {
            ang_grid[i] = i;
            auto fovex_nominal = std::pow(i,1)* (double)regs.FRMWfovexNominal[0] +
                std::pow(i, 2)* (double)regs.FRMWfovexNominal[1] +
                std::pow(i, 3)* (double)regs.FRMWfovexNominal[2] +
                std::pow(i, 4)* (double)regs.FRMWfovexNominal[3];

            ang_out_on_grid[i] = i + fovex_nominal;
        }
        std::vector<double> ang_pre_exp;
        ang_pre_exp = interp1(ang_out_on_grid, ang_grid, ang_post_exp);
        //std::vector<double> xy_norm(directions.size(), 0);
        for (auto i = 0; i < fovex_indicent_direction.size(); i++)
        {
            fovex_indicent_direction[i].z = std::cos(ang_pre_exp[i] * PI / 180.0);
            auto xy_norm = directions[i].x*directions[i].x + directions[i].y*directions[i].y;
            auto xy_factor = sqrt((1 - (fovex_indicent_direction[i].z* fovex_indicent_direction[i].z)) / xy_norm);
            fovex_indicent_direction[i].x = directions[i].x*xy_factor;
            fovex_indicent_direction[i].y = directions[i].y*xy_factor;
        }
    }

    auto x = (double)regs.FRMWlaserangleH* PI / 180.0;
    auto y = (double)(regs.FRMWlaserangleV + 180)* PI / 180.0;

    double laser_incident_direction[3] = { std::cos(y)*std::sin(x),
                                            std::sin(y),
                                            std::cos(y)*std::cos(x) };

    std::vector<double3> mirror_normal_direction(fovex_indicent_direction.size());
    std::vector<double> dsm_x_corr(fovex_indicent_direction.size());
    std::vector<double> dsm_y_corr(fovex_indicent_direction.size());
    for (auto i = 0; i < fovex_indicent_direction.size(); i++)
    {
        mirror_normal_direction[i].x = fovex_indicent_direction[i].x - laser_incident_direction[0];
        mirror_normal_direction[i].y = fovex_indicent_direction[i].y - laser_incident_direction[1];
        mirror_normal_direction[i].z = fovex_indicent_direction[i].z - laser_incident_direction[2];

        auto norm = sqrt(mirror_normal_direction[i].x*mirror_normal_direction[i].x +
            mirror_normal_direction[i].y* mirror_normal_direction[i].y +
            mirror_normal_direction[i].z* mirror_normal_direction[i].z);

        mirror_normal_direction[i].x /= norm;
        mirror_normal_direction[i].y /= norm;
        mirror_normal_direction[i].z /= norm;

        auto ang_x = std::atan(mirror_normal_direction[i].x / mirror_normal_direction[i].z)* 180.0 / PI;
        auto ang_y = std::asin(mirror_normal_direction[i].y)* 180.0 / PI;
        
        int mirror_mode = 1/*regs.frmw.mirrorMovmentMode*/;

        dsm_x_corr[i] = ang_x / (double)(regs.FRMWxfov[mirror_mode - 1] * 0.25 / 2047);
        dsm_y_corr[i] = ang_y / (double)(regs.FRMWyfov[mirror_mode - 1] * 0.25 / 2047);

    }

    std::vector<double> dsm_grid(421);

    auto ind = 0;
    for (auto i = -2100; i <= 2100; i += 10)
    {
        dsm_grid[ind++] = i;
    }

    std::vector<double> dsm_x_coarse_on_grid(dsm_grid.size());
    std::vector<double> dsm_x_corr_on_grid(dsm_grid.size());
    for (auto i = 0; i < dsm_grid.size(); i ++)
    {
        double vals[3] = { std::pow((dsm_grid[i] / 2047), 1), std::pow((dsm_grid[i] / 2047), 2), std::pow((dsm_grid[i] / 2047), 3) };
        dsm_x_coarse_on_grid[i] = dsm_grid[i] + vals[0] * (double)regs.FRMWpolyVars[0] + vals[1] * (double)regs.FRMWpolyVars[1] + vals[2] * (double)regs.FRMWpolyVars[2];
        auto val = dsm_x_coarse_on_grid[i] / 2047;
        double vals2[4] = { std::pow(val , 1),std::pow(val , 2), std::pow(val , 3) , std::pow(val , 4) };
        dsm_x_corr_on_grid[i] = dsm_x_coarse_on_grid[i] + vals2[0] * (double)regs.FRMWundistAngHorz[0] +
            vals2[1] * (double)regs.FRMWundistAngHorz[1] +
            vals2[2] * (double)regs.FRMWundistAngHorz[2] +
            vals2[3] * (double)regs.FRMWundistAngHorz[3];
    }

    auto dsm_x = interp1(dsm_x_corr_on_grid, dsm_grid, dsm_x_corr);
    std::vector<double> dsm_y(dsm_x.size());

    for (auto i = 0; i < dsm_y.size(); i++)
    {
        dsm_y[i] = dsm_y_corr[i] - (dsm_x[i] / 2047)*(double)regs.FRMWpitchFixFactor;
    }
   
    std::vector<double2> res(dsm_x.size());

    for (auto i = 0; i < res.size(); i++)
    {
        res[i].x = (dsm_x[i] + 2047) / (double)algo_calibration_registers.EXTLdsmXscale - (double)algo_calibration_registers.EXTLdsmXoffset;
        res[i].y = (dsm_y[i] + 2047) / (double)algo_calibration_registers.EXTLdsmYscale - (double)algo_calibration_registers.EXTLdsmYoffset;
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
