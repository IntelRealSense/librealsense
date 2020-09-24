// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <types.h>


TEST_CASE( "HDR ON/OFF - without streaming", "[HDR]" ) {
    try {
        rs2::context ctx;
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::device dev = devices_list[0];
        dev.hardware_reset();
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();

        std::this_thread::sleep_for((std::chrono::milliseconds)1500);

        // toggle hdr each 10 iterations
        int iterations = 0;
        float hdr_command = 0.f;
        while (++iterations < 50)
        {
            std::this_thread::sleep_for((std::chrono::milliseconds)30);

            if (hdr_command == 0.f)
                hdr_command = 1.f;
            else
                hdr_command = 0.f;

            depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, hdr_command);
            REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == hdr_command);

            std::cout << "iterations: " << iterations << " HDR is ";
            if (hdr_command)
                std::cout << "ON" << std::endl;
            else
                std::cout << "OFF" << std::endl;
        }
    }
    catch (std::exception e)
    {
        std::cout << "Exception :" << e.what() << std::endl;
    }
    
}
