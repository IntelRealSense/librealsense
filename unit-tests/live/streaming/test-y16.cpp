// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

//#test:device D400*

#include "../live-common.h"
#include <rsutils/time/timer.h>

using namespace rs2;

rs2::stream_profile get_ir_y16_profile(rs2::depth_sensor depth_sensor)
{
    auto depth_profiles = depth_sensor.get_stream_profiles();
    rs2::stream_profile ir_profile;
    for (auto& p : depth_profiles)
    {
        if (p.format() == RS2_FORMAT_Y16)
        {
            ir_profile = p;
            break;
        }
    }
    return ir_profile;
}

// Y16 CONFIGURATION TESTS
TEST_CASE("Y16 streaming", "[live]") 
{
    auto dev = find_first_device_or_exit();
    
    auto depth_sensor = dev.query_sensors()[0];
    auto y16_profile = get_ir_y16_profile(depth_sensor);
    REQUIRE(y16_profile);
    depth_sensor.open(y16_profile);
    bool y16_streamed = false;
    depth_sensor.start([&y16_streamed](rs2::frame f){
        y16_streamed = true;
    });
    rsutils::time::timer t(std::chrono::seconds(30));
    while (!t.has_expired())
    {
        if (y16_streamed)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // fail on timeout
    REQUIRE(!t.has_expired());

    depth_sensor.stop();
    depth_sensor.close();
}
