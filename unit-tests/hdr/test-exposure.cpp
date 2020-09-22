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

    depth_sensor.set_option(RS2_OPTION_SEQUENCE_SIZE, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_SIZE) == 2.f);

    // configuring id for this hdr config (value must be in range [0,3])
    float subpreset_id = 0;
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_NAME, subpreset_id);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_NAME) == subpreset_id);

    float first_exposure = 9000.f;
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, first_exposure);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == first_exposure);

    float second_exposure = 400;
    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, second_exposure);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == second_exposure);

    depth_sensor.set_option(RS2_OPTION_SEQUENCE_ID, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_SEQUENCE_ID) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    rs2::pipeline pipe;
    std::cout << "starting pipe..." << std::endl;
    pipe.start(cfg);
    std::cout << "pipe started" << std::endl;

    /*int iterations = 0;
    float pair_fc_exposure = first_exposure;
    float odd_fc_exposure = second_exposure;*/
    while (dev) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames();
        rs2::depth_frame out_depth_frame = data.get_depth_frame();

        REQUIRE(out_depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SUBPRESET_SEQUENCE_SIZE));
        REQUIRE(out_depth_frame.supports_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID));

        long long frame_counter = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        long long frame_exposure = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_ACTUAL_EXPOSURE);
        auto seq_id = out_depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
        std::cout << "seq id = " << seq_id << ", exposure = " << frame_exposure << std::endl;
        /*if (iterations++ == 0)
            continue;
        else if(iterations++ == 1)
        {
            if (frame_counter % 2 == 0)
            {
                if (frame_exposure == first_exposure)
                    continue;
                else
                {
                    pair_fc_exposure = second_exposure;
                    odd_fc_exposure = first_exposure;
                }
            }
            else
            {
                if (frame_exposure == second_exposure)
                    continue;
                else
                {
                    odd_fc_exposure = second_exposure;
                    pair_fc_exposure = first_exposure;
                }
            }
        }
        else
        {
            if (!(frame_counter % 2))
            {
                REQUIRE(frame_exposure == pair_fc_exposure);
            }
            else {
                REQUIRE(frame_exposure == odd_fc_exposure);
            }
        }
        if (iterations == 100)
            break;*/
    }
}

