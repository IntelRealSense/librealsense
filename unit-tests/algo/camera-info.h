#pragma once

#include <librealsense2/h/rs_types.h>


// Encapsulate the calibration information for a specific camera
// All the sample images we use are usually from the same camera. I.e., their
// intrinsics & extrinsics are the same and can be reused via this structure
struct camera_info
{
    rs2_intrinsics rgb;
    rs2_intrinsics z;
    rs2_extrinsics extrinsics;
    float z_units = 0.25f;
};

