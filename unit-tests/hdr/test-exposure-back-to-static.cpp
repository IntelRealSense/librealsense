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

    float exposure_before_hdr = 50.f;
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, exposure_before_hdr);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == exposure_before_hdr);

    depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0.f);

    depth_sensor.set_option(RS2_OPTION_SUBPRESET_SEQUENCE_SIZE, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SUBPRESET_SEQUENCE_SIZE) == 2.f);

    float first_exposure = 120.f;
    depth_sensor.set_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, first_exposure);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == first_exposure);

    float second_exposure = 1200.f;
    depth_sensor.set_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, second_exposure);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == second_exposure);

    depth_sensor.set_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SUBPRESET_SEQUENCE_ID) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_MODE, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_MODE) == 1.f);

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2);
    rs2::pipeline pipe;
    pipe.start(cfg);

    int iterations = 0;
    float static_exposure_back = first_exposure;
    float previous_exposure = 0.f;
    float current_exposure = 0.f;
    bool static_exposure_reached = false;
    while (dev) 
    {
        rs2::frameset data = pipe.wait_for_frames();

        rs2::depth_frame out_depth_frame = data.get_depth_frame();
        long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);

        ++iterations;
        if (iterations == 100)
        {
            depth_sensor.set_option(RS2_OPTION_HDR_MODE, 0);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_MODE) == 0.f);

            previous_exposure = frame_exposure;
        }
        else if(iterations > 100)
        {
            if (!static_exposure_reached)
            {
                current_exposure = frame_exposure;
                if (current_exposure == previous_exposure)
                {
                    static_exposure_reached = true;
                    static_exposure_back = current_exposure;
                    REQUIRE(static_exposure_back == exposure_before_hdr);
                    int iterations_from_command_to_static = iterations - 100;
                    std::cout << "iterations since command = " << iterations_from_command_to_static << std::endl;
                    REQUIRE(iterations_from_command_to_static < 10);
                }  
                else
                {
                    previous_exposure = current_exposure;
                }
            }
            else
            {
                REQUIRE(frame_exposure == static_exposure_back);
            }
        }

        if (iterations == 200)
            break;
    }

}
