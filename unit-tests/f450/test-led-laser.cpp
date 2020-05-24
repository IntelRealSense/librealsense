// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file f450-common.h

#include "f450-common.h"


TEST_CASE( "LED - LASER - no streaming", "[F450]" ) {
    using namespace std;
    
    rs2::context ctx;
    auto devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    std::vector<rs2::device> devices;

    for (auto i = 0u; i < device_count; i++)
    {
        try
        {
            auto dev = devices_list[i];
            devices.emplace_back(dev);
        }
        catch (...)
        {
            std::cout << "Failed to created device. Check SDK logs for details" << std::endl;
        }
    }


    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_RAW16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_RAW16);

    rs2::pipeline pipe;
    pipe.start(cfg);
    std::cout << "pipe started" << std::endl;
    {
        rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();

        int number_of_iterations = 5;
        float tolerance = 3; // for 5% - not absolute as done in gain

        //set AUTO EXPOSURE to auto
        sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 1);
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 1);

        rs2_option opt_led = RS2_OPTION_LED_POWER;
        rs2_option opt_laser = RS2_OPTION_LASER_POWER;

        //setting led and laser OFF
        sensor.set_option(opt_led, 0);
        REQUIRE(sensor.get_option(opt_led) == 0);
        sensor.set_option(opt_laser, 0);
        REQUIRE(sensor.get_option(opt_laser) == 0);

        bool is_led_on = false;
        bool is_laser_on = false;

        bool is_set_led = true;
        int iterations = 10;
        while (iterations--)
        {
            int tries = 5;
            if (is_set_led)
            {
                sensor.set_option(opt_led, 1);
                while (!is_led_on && tries--)
                {
                    is_led_on = (sensor.get_option(opt_led) == 1);
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                } 
                REQUIRE(is_led_on);
                REQUIRE(!is_laser_on);
            }
            else
            {
                sensor.set_option(opt_laser, 1);
                while (!is_laser_on && tries--)
                {
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                    is_led_on = (sensor.get_option(opt_led) == 1);
                }
                REQUIRE(is_laser_on);
                REQUIRE(!is_led_on);
            }
            is_set_led = !is_set_led;
        }
    }
    pipe.stop();
    std::cout << "pipe stopped" << std::endl;
    
}


TEST_CASE("LED - LASER - streaming", "[F450]") {
    using namespace std;

    rs2::context ctx;
    auto devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    std::vector<rs2::device> devices;

    for (auto i = 0u; i < device_count; i++)
    {
        try
        {
            auto dev = devices_list[i];
            devices.emplace_back(dev);
        }
        catch (...)
        {
            std::cout << "Failed to created device. Check SDK logs for details" << std::endl;
        }
    }


    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_RAW16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_RAW16);

    rs2::pipeline pipe;
    pipe.start(cfg);
    std::cout << "pipe started" << std::endl;
    {
        rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();

        int number_of_iterations = 5;
        float tolerance = 3; // for 5% - not absolute as done in gain

        //set AUTO EXPOSURE to auto
        sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 1);
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 1);

        rs2_option opt_led = RS2_OPTION_LED_POWER;
        rs2_option opt_laser = RS2_OPTION_LASER_POWER;

        //setting led and laser OFF
        sensor.set_option(opt_led, 0);
        REQUIRE(sensor.get_option(opt_led) == 0);
        sensor.set_option(opt_laser, 0);
        REQUIRE(sensor.get_option(opt_laser) == 0);
        rs2::frameset data = pipe.wait_for_frames();
        rs2::video_frame frame_0(data.get_infrared_frame(0));
        long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
        long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
        rs2::video_frame frame_1(data.get_infrared_frame(1));
        long long laser_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
        long long led_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
        REQUIRE(laser_in_md_0 == 0);
        REQUIRE(led_in_md_0 == 0);
        REQUIRE(laser_in_md_1 == 0);
        REQUIRE(led_in_md_1 == 0);


        bool is_led_on = false;
        bool is_laser_on = false;

        bool is_set_led = true;
        int iterations = 10;
        while (iterations--)
        {
            
            int tries = 5;
            if (is_set_led)
            {
                sensor.set_option(opt_led, 1);
                rs2::frameset data = pipe.wait_for_frames();

                while (!is_led_on && tries--)
                {
                    is_led_on = (sensor.get_option(opt_led) == 1);
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                }
                REQUIRE(is_led_on);
                REQUIRE(!is_laser_on);
                // get another frame so that md will be updated
                data = pipe.wait_for_frames();
                rs2::video_frame frame_0(data.get_infrared_frame(0));
                long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                rs2::video_frame frame_1(data.get_infrared_frame(1));
                long long laser_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);

                REQUIRE(led_in_md_0 == 1);
                REQUIRE(led_in_md_1 == 1);
                REQUIRE(laser_in_md_0 == 0);
                REQUIRE(laser_in_md_1 == 0);
            }
            else
            {
                sensor.set_option(opt_laser, 1);
                rs2::frameset data = pipe.wait_for_frames();
                
                while (!is_laser_on && tries--)
                {
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                    is_led_on = (sensor.get_option(opt_led) == 1);
                }
                REQUIRE(is_laser_on);
                REQUIRE(!is_led_on);
                // get another frame so that md will be updated
                data = pipe.wait_for_frames();
                rs2::video_frame frame_0(data.get_infrared_frame(0));
                long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                rs2::video_frame frame_1(data.get_infrared_frame(1));
                long long laser_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_1 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                
                REQUIRE(led_in_md_0 == 0);
                REQUIRE(led_in_md_1 == 0);
                REQUIRE(laser_in_md_0 == 1);
                REQUIRE(laser_in_md_1 == 1);
            }
            is_set_led = !is_set_led;
        }
    }
    pipe.stop();
    std::cout << "pipe stopped" << std::endl;

}