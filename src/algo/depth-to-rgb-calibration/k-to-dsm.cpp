//// License: Apache 2.0. See LICENSE file in root directory.
//// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "k-to-dsm.h"
#include "debug.h"

using namespace librealsense::algo::depth_to_rgb_calibration;


rs2_intrinsics_double rotate_k_mat(rs2_intrinsics_double k_mat)
{
    rs2_intrinsics_double res = k_mat;
    res.ppx = k_mat.width - 1 - k_mat.ppx;
    res.ppy = k_mat.height - 1 - k_mat.ppy;

    return res;
}

rs2_dsm_params librealsense::algo::depth_to_rgb_calibration::convert_new_k_to_DSM(
    rs2_intrinsics_double old_k, 
    rs2_intrinsics_double new_k, 
    rs2_dsm_params orig_dsm_params, 
    DSM_regs dsm_regs
)
{
    auto old_k_raw = rotate_k_mat(old_k);
    auto new_k_raw = rotate_k_mat(new_k);

    return rs2_dsm_params();
}

DSM_regs librealsense::algo::depth_to_rgb_calibration::apply_ac_res_on_dsm_model(rs2_dsm_params ac_data,
    DSM_regs dsm_regs,
    ac_to_dsm_dir type
)
{
    DSM_regs res;

    if (type == direct) // convert from original model to modified model
    {
        switch (ac_data.model)
        {
        case 0: // none
            res = dsm_regs;
        case 1: // AOT model
            res.dsm_x_scale = dsm_regs.dsm_x_scale*ac_data.h_scale;
            res.dsm_y_scale = dsm_regs.dsm_y_scale*ac_data.v_scale;
            res.dsm_x_offset = (dsm_regs.dsm_x_offset + ac_data.h_offset) / ac_data.h_scale;
            res.dsm_y_offset = (dsm_regs.dsm_y_offset + ac_data.v_offset) / ac_data.v_scale;
            break;
        case 2: // TOA model
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
        case 0: // none
            res = dsm_regs;
        case 1: // AOT model
            res.dsm_x_scale = dsm_regs.dsm_x_scale/ ac_data.h_scale;
            res.dsm_y_scale = dsm_regs.dsm_y_scale/ac_data.v_scale;
            res.dsm_x_offset = dsm_regs.dsm_x_offset* ac_data.h_scale - ac_data.h_offset;
            res.dsm_y_offset = dsm_regs.dsm_y_offset* ac_data.v_scale - ac_data.v_offset;
            break;
        case 2: // TOA model
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