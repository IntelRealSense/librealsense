// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#test:device D400*
//#test:device L500*
//#test:device SR300*

#include <easylogging++.h>

#include "../../catch.h"


#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>

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
    // rotation tolerance - units are in cosinus of radians
    const double rotation_tolerance = 0.00001;
    // translation tolerance - units are in meters
    const double translation_tolerance = 0.0001; // 0.1mm

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
                double tolerance = rotation_tolerance;
                if (j == 3) tolerance = translation_tolerance;

                if (fabs(pos_and_rot[i][j] - other.pos_and_rot[i][j]) > tolerance)
                {
                    std::cout << "i,j = " << i << "," << j << ", pos_and_rot[i][j] = " << pos_and_rot[i][j] << std::endl;
                    std::cout << "required value = " << other.pos_and_rot[i][j] << std::endl;
                    std::cout << "tolerance = " << tolerance << std::endl;
                    return false;
                }
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

                double tolerance = rotation_tolerance;
                if (j == 3) tolerance = translation_tolerance;

                if (fabs(pos_and_rot[i][j] - target) > tolerance)
                {
                    std::cout << "i,j = " << i << "," << j << ", pos_and_rot[i][j] = " << pos_and_rot[i][j] << std::endl;
                    std::cout << "required value = " << target << std::endl;
                    std::cout << "tolerance = " << tolerance << std::endl;
                    return false;
                }
            }
        return true;
    }
};


// This method takes in consideration that the totation array is given in the order of a column major 3x3 matrix
position_and_rotation matrix_4_by_4_from_translation_and_rotation(const float* position, const float* rotation)
{
    position_and_rotation pos_rot;
    pos_rot.pos_and_rot[0][0] = static_cast<double>(rotation[0]);
    pos_rot.pos_and_rot[1][0] = static_cast<double>(rotation[1]);
    pos_rot.pos_and_rot[2][0] = static_cast<double>(rotation[2]);

    pos_rot.pos_and_rot[0][1] = static_cast<double>(rotation[3]);
    pos_rot.pos_and_rot[1][1] = static_cast<double>(rotation[4]);
    pos_rot.pos_and_rot[2][1] = static_cast<double>(rotation[5]);

    pos_rot.pos_and_rot[0][2] = static_cast<double>(rotation[6]);
    pos_rot.pos_and_rot[1][2] = static_cast<double>(rotation[7]);
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

std::string to_string(const rs2_extrinsics& extrinsics)
{
    std::stringstream ss;
    ss << extrinsics.rotation[0] << " " << extrinsics.rotation[3] << " " << extrinsics.rotation[6] << std::endl;
    ss << extrinsics.rotation[1] << " " << extrinsics.rotation[4] << " " << extrinsics.rotation[7] << std::endl;
    ss << extrinsics.rotation[2] << " " << extrinsics.rotation[5] << " " << extrinsics.rotation[8] << std::endl;

    return ss.str();
}

const double rotation_diagonal_min = 0.95;
const double rotation_diagonal_max = 1.0;
bool check_calibration_criteria(const rs2_extrinsics& extrinsics)
{
    if (extrinsics.rotation[0] > rotation_diagonal_min && extrinsics.rotation[0] <= rotation_diagonal_max &&
        extrinsics.rotation[4] > rotation_diagonal_min && extrinsics.rotation[4] <= rotation_diagonal_max &&
        extrinsics.rotation[8] > rotation_diagonal_min && extrinsics.rotation[8] <= rotation_diagonal_max)
        return true;
    return false;
}

bool is_rgb_calibrated(const std::vector<rs2::stream_profile>& profiles)
{
    bool res = false;
    std::vector<rs2::stream_profile> depth_and_color_profiles;
    bool depth_profile_added = false;
    bool color_profile_added = false;

    for (auto&& p : profiles)
    {
        // getting depth profile
        if (!depth_profile_added && p.stream_type() == RS2_STREAM_DEPTH)
        {
            depth_and_color_profiles.push_back(p);
            depth_profile_added = true;
        }
        // getting color profile
        if (!color_profile_added && p.stream_type() == RS2_STREAM_COLOR)
        {
            depth_and_color_profiles.push_back(p);
            color_profile_added = true;
        }
        if (depth_profile_added && color_profile_added)
            break;
    }

    if (depth_and_color_profiles.size() == 2)
    {
        rs2_extrinsics depth_color_extrinsics = depth_and_color_profiles[0].get_extrinsics_to(depth_and_color_profiles[1]);
        res = check_calibration_criteria(depth_color_extrinsics);
    }

    return res;
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

        std::vector<rs2_stream> streams_to_ignore = { RS2_STREAM_ACCEL, RS2_STREAM_GYRO };
        bool rgb_calibrated = is_rgb_calibrated(profiles);
        if (!rgb_calibrated)
        {
            std::cout << "RGB is not calibrated on this device" << std::endl;
            streams_to_ignore.push_back(RS2_STREAM_COLOR);
        }
        std::vector<rs2::stream_profile> relevant_profiles;

        for (auto&& i : profiles)
        {
            auto stream_type = i.stream_type();
            auto it = find(streams_to_ignore.begin(), streams_to_ignore.end(), stream_type);
            if (it == streams_to_ignore.end())
                relevant_profiles.push_back(i);
        }

        float start_point[3] = { 1.f, 2.f, 3.f };

        for (int i = 0; i < relevant_profiles.size() - 2; ++i)
        {
            for (int j = i + 1; j < relevant_profiles.size() - 1; ++j)
            {
                rs2_extrinsics extr_i_to_j = relevant_profiles[i].get_extrinsics_to(relevant_profiles[j]);
                rs2_extrinsics extr_j_to_i = relevant_profiles[j].get_extrinsics_to(relevant_profiles[i]);

                position_and_rotation pr_i_to_j = matrix_4_by_4_from_translation_and_rotation(extr_i_to_j.translation, extr_i_to_j.rotation);
                position_and_rotation pr_j_to_i = matrix_4_by_4_from_translation_and_rotation(extr_j_to_i.translation, extr_j_to_i.rotation);

                position_and_rotation product = pr_i_to_j * pr_j_to_i;
                // checking that product of extrinsics from i to j with extrinsiscs from j to i is identity matrix
                if (!product.is_identity())
                {
                    std::cout << "i : stream type: " << relevant_profiles[i].stream_type()
                        << ", format: " << relevant_profiles[i].format()
                        << ", fps: " << relevant_profiles[i].fps()
                        << ", stream index: " << relevant_profiles[i].stream_index() << std::endl;

                    std::cout << "j : stream type: " << relevant_profiles[j].stream_type()
                        << ", format: " << relevant_profiles[j].format()
                        << ", fps: " << relevant_profiles[j].fps()
                        << ", stream index: " << relevant_profiles[j].stream_index() << std::endl;

                    std::cout << "extr_i_to_j : " << std::endl;
                    std::cout << to_string(extr_i_to_j) << std::endl;
                    std::cout << "extr_j_to_i : " << std::endl;
                    std::cout << to_string(extr_j_to_i) << std::endl;
                    std::cout << std::endl;
                }
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
                point_and_orientation.pos_and_rot[0][0] = 0.75;
                point_and_orientation.pos_and_rot[0][1] = -0.4330127;
                point_and_orientation.pos_and_rot[0][2] = 0.5;
                point_and_orientation.pos_and_rot[1][0] = 0.649519;
                point_and_orientation.pos_and_rot[1][1] = 0.625;
                point_and_orientation.pos_and_rot[1][2] = -0.4330127;
                point_and_orientation.pos_and_rot[2][0] = -0.125;
                point_and_orientation.pos_and_rot[2][1] = 0.649519;
                point_and_orientation.pos_and_rot[2][2] = 0.75;

                point_and_orientation.pos_and_rot[3][0] = 0.0;
                point_and_orientation.pos_and_rot[3][1] = 0.0;
                point_and_orientation.pos_and_rot[3][2] = 0.0;

                // translation part
                point_and_orientation.pos_and_rot[0][3] = 1.0;
                point_and_orientation.pos_and_rot[1][3] = 2.0;
                point_and_orientation.pos_and_rot[2][3] = 3.0;

                point_and_orientation.pos_and_rot[3][3] = 1.0;

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
