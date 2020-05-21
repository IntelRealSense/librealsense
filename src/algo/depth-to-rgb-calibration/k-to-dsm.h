// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#pragma once

#include "calibration.h"
#include <types.h>  // librealsense types (intr/extr)


namespace librealsense {
namespace algo {
namespace depth_to_rgb_calibration {

    struct DSM_regs
    {
        double dsm_y_offset;
        double dsm_x_offset;
        double dsm_y_scale;
        double dsm_x_scale;
    };


    DSM_regs apply_ac_res_on_dsm_model(rs2_dsm_params dsm_params, DSM_regs, rs2_dsm_correction_model type);

    rs2_dsm_params convert_new_k_to_DSM(rs2_intrinsics_double old_k, 
        rs2_intrinsics_double new_k, 
        rs2_dsm_params orig_dsm_params, 
        DSM_regs dsm_regs);


}
}
}

