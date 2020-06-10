// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//Project properties to modify so tis test will work: (TODO - add to cmake)
// - C/C++ > General > Additional Include Directories:
//   add: C:\Users\rbettan\Documents\GitRepos\librealsense2\third-party\glfw\include
// - Linker > Input > Additional Dependencies:
// add: 
//     ..\..\..\..\third-party\glfw\src\Release\glfw3.lib
//     opengl32.lib
//     glu32.lib
// Add Reference glfw

//#cmake:add-file f450-common-display.h

#include "f450-common-display.h"


TEST_CASE( "LED - LASER - streaming - not checking metadata", "[F450]" ) {
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

    rs2_option opt_led = RS2_OPTION_LED_POWER;
    rs2_option opt_laser = RS2_OPTION_LASER_POWER;

    bool isRunning = true;
    std::thread framesPuller = std::thread([&]()
        {
            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_Y16);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_Y16);

            rs2::pipeline pipe;
            pipe.start(cfg);
            std::cout << "pipe started" << std::endl;
            //------------------
            // Create a simple OpenGL window for rendering:
            window app(1280, 720, "RealSense Capture Example");

            // Declare depth colorizer for pretty visualization of depth data
            rs2::colorizer color_map;
            // Declare rates printer for showing streaming rates of the enabled streams.
            rs2::rates_printer printer;
            //------------
            while (app && isRunning)
            {
                rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
                app.show(data);
            }
            //------------
            pipe.stop();
            std::cout << "pipe stopped" << std::endl;
        });

    rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();
    {
        int number_of_iterations = 5;
        float tolerance = 3; // for 5% - not absolute as done in gain

        //setting led and laser OFF
        sensor.set_option(opt_led, 0);
        REQUIRE(sensor.get_option(opt_led) == 0);
        sensor.set_option(opt_laser, 0);
        REQUIRE(sensor.get_option(opt_laser) == 0);

        bool is_led_on = false;
        bool is_laser_on = false;

        bool is_set_led = true;
        int iterations = 500;
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

                //turn both LED and LASER OFF
                tries = 5;
                sensor.set_option(opt_led, 0);
                sensor.set_option(opt_laser, 0);
                while ((is_led_on || is_laser_on)  && tries--)
                {
                    is_led_on = (sensor.get_option(opt_led) == 1);
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                }
                REQUIRE(!is_led_on);
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

                //turn both LED and LASER OFF
                tries = 5;
                sensor.set_option(opt_led, 0);
                sensor.set_option(opt_laser, 0);
                while ((is_led_on || is_laser_on) && tries--)
                {
                    is_led_on = (sensor.get_option(opt_led) == 1);
                    is_laser_on = (sensor.get_option(opt_laser) == 1);
                }
                REQUIRE(!is_led_on);
                REQUIRE(!is_laser_on);
            }
            is_set_led = !is_set_led;
        }
    }
    isRunning = false;
    framesPuller.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //turn both OFF
    sensor.set_option(opt_led, 0);
    sensor.set_option(opt_laser, 0);
    bool is_laser_on = (sensor.get_option(opt_laser) == 1);
    bool is_led_on = (sensor.get_option(opt_led) == 1);
    REQUIRE(!is_laser_on);
    REQUIRE(!is_led_on);
}

/*
TEST_CASE("LED - LASER - streaming - checking metadata", "[F450]") {
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
    rs2::frameset* rt_data = new rs2::frameset;

    bool isRunning = true;
    std::thread framesPuller = std::thread([&](rs2::frameset* data)
        {
            rs2::config cfg;
            cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_Y16);
            cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_Y16);

            rs2::pipeline pipe;
            pipe.start(cfg);
            std::cout << "pipe started" << std::endl;
            //------------------
            // Create a simple OpenGL window for rendering:
            window app(1280, 720, "RealSense Capture Example");

            // Declare depth colorizer for pretty visualization of depth data
            rs2::colorizer color_map;
            // Declare rates printer for showing streaming rates of the enabled streams.
            rs2::rates_printer printer;
            //------------
            while (app && isRunning)
            {
                *data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
                app.show(*data);
            }
            //------------
            pipe.stop();
            std::cout << "pipe stopped" << std::endl;
        }, rt_data);

    rs2_option opt_led = RS2_OPTION_LED_POWER;
    rs2_option opt_laser = RS2_OPTION_LASER_POWER;

    rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();
    {
        int number_of_iterations = 20;
        float tolerance = 3; // for 5% - not absolute as done in gain

        //set AUTO EXPOSURE to auto
        sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 1);
        bool isAE_ON = false;
        while (!isAE_ON && number_of_iterations--)
        {
            isAE_ON = (sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 1);
        }
        REQUIRE(isAE_ON);

        //setting led and laser OFF
        sensor.set_option(opt_led, 0);
        REQUIRE(sensor.get_option(opt_led) == 0);
        sensor.set_option(opt_laser, 0);
        REQUIRE(sensor.get_option(opt_laser) == 0);

        

        rs2::frameset data = *rt_data;
        while (data.size() == 0)
        {
            std::cout << ".";
            data = *rt_data;
        }

        int a = 1;

        long long frameCounter_0 = 0;
        long long frameCounter_1 = 0;
        for (int i = 0; i < 10; ++i)
        {
            rs2::frameset data = *rt_data;
            rs2::video_frame frame_0(data.get_infrared_frame(0));
            long long current_frameCounter_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
            rs2::video_frame frame_1(data.get_infrared_frame(1));
            long long current_frameCounter_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);

            bool result = (current_frameCounter_0 > frameCounter_0) && (current_frameCounter_1 > frameCounter_1);
            if (result)
            {
                std::cout << "OK ";
            }
            else {
                std::cout << "ooooops ";
            }
            frameCounter_0 = current_frameCounter_0;
            frameCounter_1 = current_frameCounter_1;
        }
        
        /*
        rs2::video_frame frame_0(data.get_infrared_frame(0));
        long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
        long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
        rs2::video_frame frame_1(data.get_infrared_frame(1));
        long long laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
        long long led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
        bool result = (laser_in_md_0 == 0) && (led_in_md_0 == 0) && (laser_in_md_1 == 0) && (led_in_md_1 == 0);
        int tries = number_of_iterations;
        while (!result && tries--)
        {
            rs2::frameset data = *rt_data;
            rs2::video_frame frame_0(data.get_infrared_frame(0));
            laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
            led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
            rs2::video_frame frame_1(data.get_infrared_frame(1));
            laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
            led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
            result = (laser_in_md_0 == 0) && (led_in_md_0 == 0) && (laser_in_md_1 == 0) && (led_in_md_1 == 0);
        }
        REQUIRE(laser_in_md_0 == 0);
        REQUIRE(led_in_md_0 == 0);
        REQUIRE(laser_in_md_1 == 0);
        REQUIRE(led_in_md_1 == 0);
        


        bool is_led_on = false;
        bool is_laser_on = false;

        bool is_set_led = true;
        int iterations = 200;
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
                // get another frame so that md will be updated
                rs2::frameset data = *rt_data;
                rs2::video_frame frame_0(data.get_infrared_frame(0));
                long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                rs2::video_frame frame_1(data.get_infrared_frame(1));
                long long laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                bool result = (laser_in_md_0 == 0) && (led_in_md_0 == 1) && (laser_in_md_1 == 0) && (led_in_md_1 == 1);
                int tries = number_of_iterations;
                while (!result && tries--)
                {
                    rs2::frameset data = *rt_data;
                    rs2::video_frame frame_0(data.get_infrared_frame(0));
                    laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                    led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                    rs2::video_frame frame_1(data.get_infrared_frame(1));
                    laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                    led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                    result = (laser_in_md_0 == 0) && (led_in_md_0 == 1) && (laser_in_md_1 == 0) && (led_in_md_1 == 1);
                }
                if (!result)
                {
                    std::cout << "metadata wrong" << std::endl;
                }
                REQUIRE(led_in_md_0 == 1);
                REQUIRE(led_in_md_1 == 1);
                REQUIRE(laser_in_md_0 == 0);
                REQUIRE(laser_in_md_1 == 0);
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
                // get another frame so that md will be updated
                rs2::frameset data = *rt_data;
                rs2::video_frame frame_0(data.get_infrared_frame(0));
                long long laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                rs2::video_frame frame_1(data.get_infrared_frame(1));
                long long laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                long long led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                bool result = (laser_in_md_0 == 1) && (led_in_md_0 == 0) && (laser_in_md_1 == 1) && (led_in_md_1 == 0);
                int tries = number_of_iterations;
                while (!result && tries--)
                {
                    rs2::frameset data = *rt_data;
                    rs2::video_frame frame_0(data.get_infrared_frame(0));
                    laser_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                    led_in_md_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                    rs2::video_frame frame_1(data.get_infrared_frame(1));
                    laser_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_FRAME_LASER_POWER_MODE);
                    led_in_md_1 = frame_1.get_frame_metadata(RS2_FRAME_METADATA_LED_POWER_MODE);
                    result = (laser_in_md_0 == 1) && (led_in_md_0 == 0) && (laser_in_md_1 == 1) && (led_in_md_1 == 0);
                }
                if (!result)
                {
                    std::cout << "metadata wrong" << std::endl;
                }
                REQUIRE(led_in_md_0 == 0);
                REQUIRE(led_in_md_1 == 0);
                REQUIRE(laser_in_md_0 == 1);
                REQUIRE(laser_in_md_1 == 1); 
            }
            is_set_led = !is_set_led;
        }
    }
    isRunning = false;
    framesPuller.join();
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    //turn both OFF
    sensor.set_option(opt_led, 0);
    sensor.set_option(opt_laser, 0);
    bool is_laser_on = (sensor.get_option(opt_laser) == 1);
    bool is_led_on = (sensor.get_option(opt_led) == 1);
    REQUIRE(!is_laser_on);
    REQUIRE(!is_led_on);
}*/