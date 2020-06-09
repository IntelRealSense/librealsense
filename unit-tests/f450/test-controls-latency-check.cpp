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
#include <types.h>


bool check_with_tolerance(float current_value, float required_value, float tolerance = 5)
{
    return (current_value >= (required_value - tolerance)) && (current_value <= (required_value + tolerance));
}


TEST_CASE( "Gain latency check while streaming", "[F450]" ) {
    using namespace std;
    rs2_error* e;
    rs2_log_to_console(RS2_LOG_SEVERITY_DEBUG, &e);

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

    rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();

    int number_of_iterations = 5;
    float tolerance = 2;
    //set AUTO EXPOSURE to manual
    sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 0);
    REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

    rs2_option opt = RS2_OPTION_GAIN;
    float gain_first = 20.0f;
    float gain_second = 110.0f;
    float gain_thirs = 220.0f;

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
    int framesCounter = 0;
    float required_value = 0.f;
    bool has_requested_changed = false;
    int frameCounterAtRequest = 0;
    while (app)
    {
        rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        app.show(data);
        ++framesCounter;
        if (framesCounter == 10)
        {
            auto timeBeforeSet = std::chrono::system_clock::now();
            sensor.set_option(opt, gain_first);
            auto timeAfterSet = std::chrono::system_clock::now();
            std::chrono::duration<double> dur = timeAfterSet - timeBeforeSet;

            std::cout << "start of set action = " << std::chrono::system_clock::to_time_t(timeBeforeSet) 
                << "time of set action = " << dur.count() * 1000.0 << "ms" << std::endl;
            std::stringstream s;
            s << "test sending set_option for gain, with value: " << gain_first << "\n";
            LOG_DEBUG(s.str());
            frameCounterAtRequest = 10;
            required_value = gain_first;
            has_requested_changed = true;
        }

        if (framesCounter == 20)
        {
            auto timeBeforeSet = std::chrono::system_clock::now();
            sensor.set_option(opt, gain_second);
            auto timeAfterSet = std::chrono::system_clock::now();
            std::chrono::duration<double> dur = timeAfterSet - timeBeforeSet;

            std::cout << "start of set action = " << std::chrono::system_clock::to_time_t(timeBeforeSet)
                << ", time of set action = " << dur.count() * 1000.0 << "ms" << std::endl;
            std::stringstream s;
            s << "test sending set_option for gain, with value: " << gain_second << "\n";
            LOG_DEBUG(s.str());
            frameCounterAtRequest = 20;
            required_value = gain_second;
            has_requested_changed = true;
        }

        rs2::video_frame frame_0(data.get_infrared_frame(0));
        long long current_gain_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_GAIN_LEVEL);
        long long current_frame_counter_0 = frame_0.get_frame_metadata(RS2_FRAME_METADATA_FRAME_COUNTER);
        
        auto frameArrivingTime = std::chrono::system_clock::now();
        auto frameArrivingTime_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(frameArrivingTime);
        auto value = frameArrivingTime_ns.time_since_epoch();
        long duration = value.count();

        std::chrono::nanoseconds dur(duration);
        cout << "*******frameCounter = " << framesCounter 
            <<  ", libRS frameCounter = " << frame_0.get_frame_number()
            << ", in md: " << current_frame_counter_0 << " - ";
        cout << "current_value = " << current_gain_0
            << ",  required_value = " << required_value<< " - ";
        cout << "frame arriving to test time = " << duration << endl;
        
        if (has_requested_changed)
        {
            if (check_with_tolerance(current_gain_0, required_value))
            {
                has_requested_changed = false;
                cout << ", latency = " << framesCounter - frameCounterAtRequest << endl;
            }
        }
    }
    //------------
    pipe.stop();
    std::cout << "pipe stopped" << std::endl;


    
}
