// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

/////////////////////////////////////////////////////////////////////////////
// This set of tests is valid for any device that supports the HDR feature //
/////////////////////////////////////////////////////////////////////////////

#define CATCH_CONFIG_MAIN
#include "../../catch.h"
#include "../func-common.h"
#include <librealsense2/rsutil.h>

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

using namespace rs2;

// structure of a matrix 4 X 4, representing rotation and translation as following:
// pos_and_rot[i][j] is 
//  _                        _ 
// |           |              |
// | rotation  | translation  |
// |   (3x3)   |    (3x1)     |
// | _________ |____________  |
// |     0     |      1       |
// |_  (1x3)   |    (1x1)    _|
//
struct position_and_rotation {
    double pos_and_rot[4][4];

    position_and_rotation operator* (const position_and_rotation& other)
    {
        position_and_rotation product;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                product.pos_and_rot[i][j] = 0;
                for (int k = 0; k < 4; k++)
                    product.pos_and_rot[i][j] += pos_and_rot[i][k] * other.pos_and_rot[k][j];
            }
        }
        return product;
    }

    bool equals(const position_and_rotation& other)
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
            {
                double tolerance = 0.0000001;
                // lower tolerance needed for translation
                if (j == 3) tolerance = 0.0005;

                if (fabs(pos_and_rot[i][j] - other.pos_and_rot[i][j]) > tolerance)
                    return false;
            }
        return true;
    }
    
    bool is_identity()
    {
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
            {
                double target = 0.0;
                if (i == j) target = 1.0;

                double tolerance = 0.0000001;
                // lower tolerance needed for translation
                if (j == 3) tolerance = 0.0005;

                if (fabs(pos_and_rot[i][j] - target) > tolerance)
                    return false;
            }
        return true;
    }
};
position_and_rotation matrix_4_by_4_from_translation_and_rotation(const float* position, const float* rotation)
{
    position_and_rotation pos_rot;
    pos_rot.pos_and_rot[0][0] = static_cast<double>(rotation[0]);
    pos_rot.pos_and_rot[0][1] = static_cast<double>(rotation[1]);
    pos_rot.pos_and_rot[0][2] = static_cast<double>(rotation[2]);

    pos_rot.pos_and_rot[1][0] = static_cast<double>(rotation[3]);
    pos_rot.pos_and_rot[1][1] = static_cast<double>(rotation[4]);
    pos_rot.pos_and_rot[1][2] = static_cast<double>(rotation[5]);

    pos_rot.pos_and_rot[2][0] = static_cast<double>(rotation[6]);
    pos_rot.pos_and_rot[2][1] = static_cast<double>(rotation[7]);
    pos_rot.pos_and_rot[2][2] = static_cast<double>(rotation[8]);

    pos_rot.pos_and_rot[3][0] = 0.0;
    pos_rot.pos_and_rot[3][1] = 0.0;
    pos_rot.pos_and_rot[3][2] = 0.0;

    pos_rot.pos_and_rot[0][3] = static_cast<double>(position[0]);
    pos_rot.pos_and_rot[1][3] = static_cast<double>(position[1]);
    pos_rot.pos_and_rot[2][3] = static_cast<double>(position[2]);

    pos_rot.pos_and_rot[3][3] = 1.0;

    return pos_rot;
}



// checking the extrinsics graph
// steps are:
// 1. get all the profiles
// 2. for each pair of these profiles: 
//    check that:
//    point P(x,y,z,head,pitch,roll) in profile A transformed to profile B coordinates, and then transformed back to profile A coordinates == same point P
TEST_CASE("Extrinsics graph - matrices 4x4", "[live]")
{
    rs2::context ctx;
    auto list = ctx.query_devices();

    for (auto&& device : list)
    {
        std::cout << "device: " << device.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
        auto sensors = device.query_sensors();

        // getting stream profiles from all sensors
        std::vector<rs2::stream_profile> profiles;
        for (auto&& sensor : sensors)
        {
            std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();
            for (auto&& profile : stream_profiles)
            {
                profiles.push_back(profile);
            }
        }

        float start_point[3] = { 1.f, 2.f, 3.f };

        for (int i = 0; i < profiles.size() - 2; ++i)
        {
            for (int j = i + 1; j < profiles.size() - 1; ++j)
            {
                rs2_extrinsics extr_i_to_j = profiles[i].get_extrinsics_to(profiles[j]);
                rs2_extrinsics extr_j_to_i = profiles[j].get_extrinsics_to(profiles[i]);

                position_and_rotation pr_i_to_j = matrix_4_by_4_from_translation_and_rotation(extr_i_to_j.translation, extr_i_to_j.rotation);
                position_and_rotation pr_j_to_i = matrix_4_by_4_from_translation_and_rotation(extr_j_to_i.translation, extr_j_to_i.rotation);

                position_and_rotation product = pr_i_to_j * pr_j_to_i;
                // checking that product of extrinsics from i to j with extrinsiscs from j to i is identity matrix
                REQUIRE(product.is_identity());

                // checking with API rs2_transform_point_to_point
                float transformed_point[3];
                rs2_transform_point_to_point(transformed_point, &extr_i_to_j, start_point);
                float end_point[3];
                rs2_transform_point_to_point(end_point, &extr_j_to_i, transformed_point);
                bool res = true;
                for (int t = 0; t < 3; ++t)
                {
                    if (fabs(start_point[t] - end_point[t]) > 0.001f)
                        res = false;
                }
                // checking that transforming a point with extrinsiscs from i to j and the from j to i
                // gets back to the initial point
                REQUIRE(res);

                // checking that a point and orientation, represented by a 4x4 matrix
                // is equal to the result of transforming it by the extrinsics 
                // from profile A to B, and then from profile B to A
                position_and_rotation point_and_orientation;
                // rotation part with 30 degrees rotation on each axis
                point_and_orientation.pos_and_rot[0][0] = 0.75f;
                point_and_orientation.pos_and_rot[0][1] = -0.4330127f;
                point_and_orientation.pos_and_rot[0][2] = 0.5f;
                point_and_orientation.pos_and_rot[1][0] = 0.649519f;
                point_and_orientation.pos_and_rot[1][1] = 0.625f;
                point_and_orientation.pos_and_rot[1][2] = -0.4330127f;
                point_and_orientation.pos_and_rot[2][0] = -0.125f;
                point_and_orientation.pos_and_rot[2][1] = 0.649519f;
                point_and_orientation.pos_and_rot[2][2] = 0.75f;

                point_and_orientation.pos_and_rot[3][0] = 0.f;
                point_and_orientation.pos_and_rot[3][1] = 0.f;
                point_and_orientation.pos_and_rot[3][2] = 0.f;

                // translation part
                point_and_orientation.pos_and_rot[0][3] = 1.f;
                point_and_orientation.pos_and_rot[1][3] = 2.f;
                point_and_orientation.pos_and_rot[2][3] = 3.f;

                point_and_orientation.pos_and_rot[3][3] = 1.f;

                // applying extrinsics from i to j on point with orientation
                position_and_rotation retransformed_temp = pr_j_to_i * point_and_orientation;
                // applying extrinsics from j to i
                position_and_rotation retransformed = pr_i_to_j * retransformed_temp; 
                // checking that the point and orientation are the same as before the transformations
                REQUIRE(retransformed.equals(point_and_orientation));
            }
        }
    }
}
