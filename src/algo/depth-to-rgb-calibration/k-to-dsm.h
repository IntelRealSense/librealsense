// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration.h"
#include "../../../include/librealsense2/h/rs_types.h"
#include <types.h>  // librealsense types (intr/extr)


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    struct DSM_regs
    {
        float dsm_x_scale;
        float dsm_y_scale;
        float dsm_x_offset;
        float dsm_y_offset;
    }; 

    enum ac_to_dsm_dir
    {
        direct,
        inverse
    };

    enum dsm_model
    {
        none = 0,
        AOT = 1,
        TOA = 2
    };

    struct los_shift_scaling
    {
        float los_scaling_x;
        float los_scaling_y;
        float los_shift_x;
        float los_shift_y;
    };

    struct DEST
    {

    };

    struct DIGG
    {

    };

    struct EXTL
    {

    };

    struct FRMW
    {
        bool fovex_existence_flag = 1;
        float fovex_nominal[4] = { 0.080740497, 0.0030212000, -0.00012760000, 3.5999999e-06 };
        float laserangleH = 1.0480000;
        float laserangleV = -0.3659388;
        double mirrorMovmentMode = 1;
        float xfov[5] = { 66.056244, 66.056244, 66.056244, 66.056244, 66.056244 };
        float yfov[5] = { 56.511749, 56.511749, 56.511749, 56.511749, 56.511749 };
        float polyVars[3] = { 0, 74.501328, 0 };
        float undistAngHorz[4] = { 3.8601303, -21.056725, -12.682472, 39.103699 };
        float pitchFixFactor = 66.7550049;
    };

    struct regs
    {
        DEST dest;
        DIGG digg;
        EXTL extl;
        FRMW frmw;
    };

    struct pre_process_data
    {
        los_shift_scaling last_los_error;
        std::vector<double3> vertices_orig;
        std::vector<uint8_t> relevant_pixels_image_rot;

    };

    class k_to_DSM
    {
    public:
        DSM_regs apply_ac_res_on_dsm_model(const rs2_dsm_params& ac_data, const DSM_regs& regs, const ac_to_dsm_dir& type);

        los_shift_scaling convert_ac_data_to_los_error(const DSM_regs& dsm_regs, const rs2_dsm_params& ac_data);

        los_shift_scaling convert_norm_vertices_to_los(const regs& regs, 
            const DSM_regs& dsm_regs, 
            const std::vector<double3>& vertices);

        pre_process_data pre_processing(const regs& regs,
            const rs2_dsm_params& ac_data,
            const DSM_regs& dsm_regs,
            const rs2_intrinsics_double& k_raw,
            const std::vector<uint8_t>& relevant_pixels_image);

        rs2_dsm_params convert_new_k_to_DSM(const rs2_intrinsics_double& old_k,
            const rs2_intrinsics_double& new_k,
            const rs2_dsm_params& orig_dsm_params,
            const DSM_regs& dsm_regs,
            const regs& regs,
            const std::vector<uint8_t>& relevant_pixels_image);

        const pre_process_data& get_pre_process_data() const;

    private:
        std::vector<double3> calc_relevant_vertices(const std::vector<uint8_t>& relevant_pixels_image, const rs2_intrinsics_double& k);
        los_shift_scaling convert_norm_vertices_to_los(const regs& regs, const DSM_regs& dsm_regs, std::vector<double3> vertices);
        std::vector<double3> transform_to_direction(std::vector<double3>);
        
        pre_process_data _pre_process_data;
    };
   


}
}
}

