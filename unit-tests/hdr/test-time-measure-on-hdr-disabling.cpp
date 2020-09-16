// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <chrono>
#include <types.h>


TEST_CASE( "HDR Running - restart hdr at restream", "[HDR]" ) {
    
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    rs2::device dev = devices_list[0];  //1
    rs2::depth_sensor depth_sensor = dev.query_sensors().front();

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    rs2::pipeline pipe;
    pipe.start(cfg);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

    for (int i = 0; i < 100; ++i)
    {
        if (i == 50)
        {
            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 0.f);
            auto startTime = std::chrono::high_resolution_clock::now();
            rs2::frameset data = pipe.wait_for_frames();
            auto get_frame_after_hdr_disabled = std::chrono::high_resolution_clock::now();
            auto get_frame_after_hdr_disabled_duration = std::chrono::duration_cast<std::chrono::microseconds>(get_frame_after_hdr_disabled - startTime);
            std::cout << "time for getting frame after hdr disabling = " << get_frame_after_hdr_disabled_duration.count() << " microseconds" << std::endl;
        }
        else
        {
            rs2::frameset data = pipe.wait_for_frames();
            rs2::depth_frame out_depth_frame = data.get_depth_frame();
        }
        
    }

    pipe.stop();   

}
