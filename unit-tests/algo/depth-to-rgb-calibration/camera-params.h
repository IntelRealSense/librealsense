#pragma once

#include <librealsense2/h/rs_types.h>
#include "../../../src/algo/depth-to-rgb-calibration/calibration.h"
namespace algo = librealsense::algo::depth_to_rgb_calibration;
// Encapsulate the calibration information for a specific camera
// All the sample images we use are usually from the same camera. I.e., their
// intrinsics & extrinsics are the same and can be reused via this structure
struct camera_info
{
    algo::rs2_intrinsics_double rgb;
    algo::rs2_intrinsics_double z;
    algo::rs2_extrinsics_double extrinsics;
    double z_units = 0.25f;
};

