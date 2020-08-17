// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <types.h>


TEST_CASE( "HDR Config - changing only exposure", "[HDR]" ) {
    
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    rs2::device dev = devices_list[0];
    rs2::depth_sensor depth_sensor = dev.query_sensors().front();

    depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_SIZE, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_SIZE) == 2.f);
    
    depth_sensor.set_option(RS2_OPTION_HDR_RELATIVE_MODE, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_RELATIVE_MODE) == 0.f);

    float first_gain = 90.f;
    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, first_gain);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == first_gain);

    float second_gain = 20.f;
    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, second_gain);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == second_gain);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2);
    rs2::pipeline pipe;
    pipe.start(cfg);

    int iterations = 0;
    float static_gain_back = first_gain;
    float previous_gain = 0.f;
    float current_gain = 0.f;
    bool static_gain_reached = false;
    while (dev) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames();

        rs2::depth_frame out_depth_frame = data.get_depth_frame();
        long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);

        ++iterations;
        if (iterations == 100)
        {
            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 0);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 0.f);

            previous_gain = frame_gain;
        }
        else if(iterations > 100)
        {
            if (!static_gain_reached)
            {
                current_gain = frame_gain;
                if (current_gain == previous_gain)
                {
                    static_gain_reached = true;
                    static_gain_back = current_gain;
                    int iterations_from_command_to_static = iterations - 100;
                    std::cout << "iterations since command = " << iterations_from_command_to_static << std::endl;
                    REQUIRE(iterations_from_command_to_static < 10);
                }  
                else
                {
                    previous_gain = current_gain;
                }
            }
            else
            {
                REQUIRE(frame_gain == static_gain_back);
            }
        }

        if (iterations == 200)
            break;
    }

}
