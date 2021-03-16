// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "../algo-common.h"
#include <librealsense2/rsutil.h>

void compare(float pixel1[2], float pixel2[2])
{
    for (auto i = 0; i < 3; i++)
    {
        REQUIRE(std::abs(pixel1[0] - pixel2[0]) < 0.0001);
    }
}

TEST_CASE( "inverse_brown_conrady_deproject" )
{
    float point[3] = { 0 };

    rs2_intrinsics intrin
        = { 1280,
            720,
            643.720581,
            357.821259,
            904.170471,
            905.155090,
            RS2_DISTORTION_INVERSE_BROWN_CONRADY,
            { 0.180086836, -0.534179211, -0.00139013783, 0.000118769123, 0.470662683 } };
    float pixel1[2] = { 1, 1 };
    float pixel2[2] = { 0, 0 };
    float depth = 10.5;
    rs2_deproject_pixel_to_point( point, &intrin, pixel1, depth );
    rs2_project_point_to_pixel( pixel2, &intrin, point );

    compare(pixel1, pixel2);
}

TEST_CASE( "brown_conrady_deproject" )
{
    float point[3] = { 0 };

    rs2_intrinsics intrin
        = { 1280,
            720,
            643.720581,
            357.821259,
            904.170471,
            905.155090,
            RS2_DISTORTION_BROWN_CONRADY,
            { 0.180086836, -0.534179211, -0.00139013783, 0.000118769123, 0.470662683 } };
    float pixel1[2] = { 1, 1 };
    float pixel2[2] = { 0, 0 };
    float depth = 10.5;
    rs2_deproject_pixel_to_point( point, &intrin, pixel1, depth );
    rs2_project_point_to_pixel( pixel2, &intrin, point );

    compare(pixel1, pixel2);
}
