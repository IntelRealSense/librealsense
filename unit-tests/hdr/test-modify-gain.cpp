// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <types.h>


TEST_CASE( "HDR Modify - changing only gain", "[HDR]" ) {
    
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    rs2::device dev = devices_list[0];
    rs2::depth_sensor depth_sensor = dev.query_sensors().front();

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

    depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_SIZE) == 2.f);

    float first_gain = 90.f;
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, first_gain);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == first_gain);

    float second_gain = 20.f;
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, second_gain);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == second_gain);

    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 0.f);


    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    cfg.enable_stream(RS2_STREAM_INFRARED, 2);
    rs2::pipeline pipe;
    pipe.start(cfg);

    int iterations = 0;
    float pair_fc_gain = first_gain;
    float odd_fc_gain = second_gain;
    while (dev) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames();

        rs2::depth_frame out_depth_frame = data.get_depth_frame();
        long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        long long frame_gain = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);

        static long long frame_counter_s = frame_counter;
        if (iterations++ == 0)
        {
            if (frame_counter % 2 == 0)
            {
                if (frame_gain == first_gain)
                    continue;
                else
                {
                    pair_fc_gain = second_gain;
                    odd_fc_gain = first_gain;
                }
            }
            else
            {
                if (frame_gain == second_gain)
                    continue;
                else
                {
                    pair_fc_gain = second_gain;
                    odd_fc_gain = first_gain;
                }
            }
        }
        else
        {
            if (!(frame_counter % 2))
            {
                REQUIRE(frame_gain == pair_fc_gain);
            }
            else {
                REQUIRE(frame_gain == odd_fc_gain);
            }
        }
        if (iterations == 100)
            break;
    }

}
