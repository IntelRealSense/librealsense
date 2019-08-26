// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_sensor.hpp>
#include "../../common/tiny-profiler.h"
#include "./../src/environment.h"

using namespace librealsense;
using namespace librealsense::platform;

// Require that vector is exactly the zero vector
inline void require_zero_vector(const float(&vector)[3])
{
    for (int i = 1; i < 3; ++i) REQUIRE(vector[i] == 0.0f);
}

// Require that matrix is exactly the identity matrix
inline void require_identity_matrix(const float(&matrix)[9])
{
    static const float identity_matrix_3x3[] = { 1,0,0, 0,1,0, 0,0,1 };
    for (int i = 0; i < 9; ++i) REQUIRE(matrix[i] == Approx(identity_matrix_3x3[i]));
}

TEST_CASE("Extrinsic graph management", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        // For each device
        auto& b = environment::get_instance().get_extrinsics_graph();
        for (int i = 0; i < 5000; i++)
        {
            //for (auto&& dev : list)
            for (int j=0; j < list.size(); j++)
            {
                rs2::device dev{};
                {
                    //scoped_timer t1("Dev generation");
                    dev = list[j];
                }

                std::cout << __LINE__ << " Iteration " << i << " : Extrinsic graph map size is " << b._streams.size() << std::endl;
                for (auto&& snr : dev.query_sensors())
                {
                    std::vector<rs2::stream_profile> profs;
                    REQUIRE_NOTHROW(profs = snr.get_stream_profiles());
                    //snr.get_info(RS2_CAMERA_INFO_NAME);
                    REQUIRE(profs.size() > 0);

                    //std::cout << __LINE__ << " " << snr.get_info(RS2_CAMERA_INFO_NAME) <<" : Extrinsic graph map size is " << b._streams.size() << std::endl;

                    //rs2_extrinsics extrin{};
                    //try {
                    //    auto prof = profs[0];
                    //    extrin = prof.get_extrinsics_to(prof);
                    //}
                    //catch (const rs2::error &e) {
                    //    // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                    //    WARN(e.what());
                    //    continue;
                    //}

                    /*require_identity_matrix(extrin.rotation);
                    require_zero_vector(extrin.translation);*/
                }
            }
        }
    }
}
