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

TEST_CASE("Extrinsic graph management", "[live][multicam]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    {
        std::cout << "Extrinsic graph management started" << std::endl;
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        std::map<std::string, size_t> extrinsic_graph_at_sensor;
        auto& b = environment::get_instance().get_extrinsics_graph();
        auto init_size = b._streams.size();
        std::cout << " Initial Extrinsic Graph size is " << init_size << std::endl;

        for (int i = 0; i < 10; i++)
        {
            std::cout << "Iteration " << i << " : Extrinsic graph map size is " << b._streams.size() << std::endl;
            // For each device
            for (auto&& dev : list)
            {
                //std::cout << "Dev " << dev.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
                for (auto&& snr : dev.query_sensors())
                {
                    std::vector<rs2::stream_profile> profs;
                    REQUIRE_NOTHROW(profs = snr.get_stream_profiles());
                    REQUIRE(profs.size() > 0);

                    std::string snr_id = snr.get_info(RS2_CAMERA_INFO_NAME);
                    snr_id += snr.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                    if (extrinsic_graph_at_sensor.count(snr_id))
                    {
                        CAPTURE(snr_id);
                        CAPTURE(extrinsic_graph_at_sensor.at(snr_id));
                        CAPTURE(b._streams.size());
                        REQUIRE(b._streams.size() == extrinsic_graph_at_sensor.at(snr_id));
                    }
                    else
                        extrinsic_graph_at_sensor[snr_id] = b._streams.size();

//                    std::cout << __LINE__ << " " << snr.get_info(RS2_CAMERA_INFO_NAME) <<" : Extrinsic graph map size is " << b._streams.size() << std::endl;

                    rs2_extrinsics extrin{};
                    try {
                        auto prof = profs[0];
                        extrin = prof.get_extrinsics_to(prof);
                    }
                    catch (const rs2::error &e) {
                        // if device isn't calibrated, get_extrinsics must error out (according to old comment. Might not be true under new API)
                        WARN(e.what());
                        continue;
                    }

                    require_identity_matrix(extrin.rotation);
                    require_zero_vector(extrin.translation);
                }
            }
        }

        auto end_size = b._streams.size();
        std::cout << " Final Extrinsic Graph size is " << end_size << std::endl;
        //REQUIRE(end_size == init_size); TODO doesn't pass yet
        WARN("TODO: Graph size shall be preserved: init " << init_size << " != final " << end_size);
    }
}
