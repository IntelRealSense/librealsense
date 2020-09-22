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

    depth_sensor.set_option(RS2_OPTION_EMITTER_ON_OFF, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ON_OFF) == 1.f);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

    rs2::sequence_id_filter split(1);

    int iteration = 0;
    while (++iteration < 50) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames();
        data = split.process(data);
        auto depth_frame = data.get_depth_frame();

        auto seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
        std::cout << "iteration: " << iteration << ", depth seq_id = " << seq_id;

        auto ir_frame = data.get_infrared_frame(1);

        auto ir_seq_id = ir_frame.get_frame_metadata(RS2_FRAME_METADATA_SEQUENCE_ID);
        std::cout << ", infrared seq_id = " << ir_seq_id << std::endl;
        std::cout << std::endl;
    }
    pipe.stop();
}

