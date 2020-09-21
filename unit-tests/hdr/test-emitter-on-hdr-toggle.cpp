// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <types.h>


TEST_CASE("HDR Config - changing only exposure", "[HDR]") {

    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    rs2::device dev = devices_list[0];
    rs2::depth_sensor depth_sensor = dev.query_sensors().front();

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    rs2::pipeline pipe;
    std::cout << "starting pipe..." << std::endl;
    pipe.start(cfg);
    std::cout << "pipe started" << std::endl;

    depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED) == 1.f);


    int iterations = 0;
    float hdr_command = 0.f;
    while (dev) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames();

        ++iterations;
        if (hdr_command == 0.f)
            hdr_command = 1.f;
        else
            hdr_command = 0.f;

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, hdr_command);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == hdr_command);

        REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED) == 1.f);

        std::cout << "iteration no " << iterations << " - emitter is ON, HDR is ";
        if (hdr_command)
            std::cout << "ON" << std::endl;
        else
            std::cout << "OFF" << std::endl;

        if (iterations == 100)
            break;
    }
    pipe.stop();
}

