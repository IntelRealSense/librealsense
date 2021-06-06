// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "unit-tests-common.h"
#include <librealsense2/rsutil.h>

// C++ test to check that rsutil function 
TEST_CASE( "rsutil-api", "[live][rsutil-api]" )
{
    rs2::context ctx;

    if (make_context(SECTION_FROM_TEST_NAME, &ctx))
    {
        float pixel[2] = { 0.5 };
        float point[3] = { 0.5 };
        rs2_intrinsics intrinsic = { 1, 1, 0.5, 0.5, 0.5 , 0.5, RS2_DISTORTION_NONE, { 0.0, 0.0, 0.0, 0.0, 0.0 } };
        float rotation[9] = { 0.0 };
        float translation[3] = { 0.0 };
        rs2_extrinsics extrinsic = { *rotation ,*translation };

        CHECK_NOTHROW(rs2_project_point_to_pixel(pixel, &intrinsic, point));

        CHECK_NOTHROW(rs2_deproject_pixel_to_point(point, &intrinsic, pixel, 0.0));

        CHECK_NOTHROW(rs2_transform_point_to_point(point, &extrinsic, point));

        CHECK_NOTHROW(rs2_fov(&intrinsic, pixel));

        CHECK_NOTHROW(rs2_project_color_pixel_to_depth_pixel(pixel, 0, 0.5, 0.0, 1.0, &intrinsic, &intrinsic, &extrinsic, &extrinsic, pixel));
    }
}
