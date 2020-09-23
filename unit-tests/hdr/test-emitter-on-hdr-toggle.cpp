// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h
//#cmake:add-file example.hpp

#include "hdr-common.h"
#include <types.h>
#include "example.hpp"


TEST_CASE("HDR Config - changing only exposure", "[HDR]") {


    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "test_emitter_hdr_toggle.txt");


    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    rs2::device dev = devices_list[0];

    dev.hardware_reset();

    rs2::depth_sensor depth_sensor = dev.query_sensors().front();


    //----------------------------------------
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 33000.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 33000.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 90.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 90.f);


    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 1.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 20.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 20.f);
    //----------------------------------------*/

    depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 1.f);


    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

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
    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames().    // Wait for next set of frames from the camera
            apply_filter(color_map);   // Find and colorize the depth data

        app.show(data);

        ++iterations;
        // toggle hdr each 10 iterations
        if (iterations % 30 == 0)
        {
            if (hdr_command == 0.f)
                hdr_command = 1.f;
            else
                hdr_command = 0.f;

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, hdr_command);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == hdr_command);
        }
        

        REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED) == 1.f);
        
        auto depth_frame = data.get_depth_frame();
        auto laser_on = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
        REQUIRE(laser_on == 1.f);
        auto laser_power = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER);
        auto ae = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_AUTO_EXPOSURE);
        auto exp = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

        std::cout << "iteration no " << iterations << " - emitter is ON, power = " << laser_power<< " , HDR is ";
        if (hdr_command)
            std::cout << "ON";
        else
            std::cout << "OFF";

        std::cout << ", AE = " << ae << ", exosure = " << exp << std::endl;
        if (iterations == 500)
            break;
    }
    pipe.stop();
}

