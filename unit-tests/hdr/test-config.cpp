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

    //depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
    //REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_SIZE, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_SIZE) == 2.f);
    
    depth_sensor.set_option(RS2_OPTION_HDR_RELATIVE_MODE, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_RELATIVE_MODE) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 1.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 120.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 120.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 90.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 90.f);


    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 2);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 2.f);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1200.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_EXPOSURE) == 1200.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 20.f);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_GAIN) == 20.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 0);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_SEQUENCE_ID) == 0.f);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);
    REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

}
