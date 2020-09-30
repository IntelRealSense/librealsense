// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.


//#cmake:add-file hdr-common.h

#include "hdr-common.h"
#include <types.h>

std::string last_option_set;
std::string last_warning_received;

TEST_CASE( "HDR ON - set locked options", "[HDR]" ) {
    try {
        auto callback = [&](rs2_log_severity severity, rs2::log_message const& msg)
        {
            last_warning_received = msg.raw();
        };
        rs2::log_to_callback(RS2_LOG_SEVERITY_WARN, callback);
        rs2::context ctx;
        rs2::device_list devices_list = ctx.query_devices();
        size_t device_count = devices_list.size();
        REQUIRE(device_count > 0);

        rs2::device dev = devices_list[0];
        dev.hardware_reset();
        rs2::depth_sensor depth_sensor = dev.query_sensors().front();


        //setting laser ON
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f);
        auto laser_power_before_hdr = depth_sensor.get_option(RS2_OPTION_LASER_POWER);

        std::this_thread::sleep_for((std::chrono::milliseconds)(1500));

        depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_HDR_ENABLED) == 1.f);

        // the following calls should not be performed and should send a LOG_WARNING
        last_option_set = "RS2_OPTION_ENABLE_AUTO_EXPOSURE";
        depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE) == 0.f);
        REQUIRE(last_warning_received == "Auto Exposure cannot be set while HDR is enabled");

        last_option_set = "RS2_OPTION_EMITTER_ENABLED";
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ENABLED) == 1.f);
        REQUIRE(last_warning_received == "Emitter status cannot be set while HDR is enabled");

        last_option_set = "RS2_OPTION_EMITTER_ON_OFF";
        depth_sensor.set_option(RS2_OPTION_EMITTER_ON_OFF, 1.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_EMITTER_ON_OFF) == 0.f);
        REQUIRE(last_warning_received == "Emitter ON/OFF cannot be set while HDR is enabled");

        last_option_set = "RS2_OPTION_LASER_POWER";
        depth_sensor.set_option(RS2_OPTION_LASER_POWER, laser_power_before_hdr - 30.f);
        REQUIRE(depth_sensor.get_option(RS2_OPTION_LASER_POWER) == laser_power_before_hdr);
        REQUIRE(last_warning_received == "Laser Power status cannot be set while HDR is enabled");


    }
    catch (std::exception e)
    {
        std::cout << "Exception :" << last_option_set << ", " << e.what() << std::endl;
        exit(1);
    }
    
}
