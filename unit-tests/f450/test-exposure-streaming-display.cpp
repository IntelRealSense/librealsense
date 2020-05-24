// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//Project properties to modify so tis test will work: (TODO - add to cmake)
// - C/C++ > General > Additional Include Directories:
//   add: C:\Users\rbettan\Documents\GitRepos\librealsense2\third-party\glfw\include
// - Linker > Input > Additional Dependencies:
// add: 
//     ..\..\..\..\third - party\glfw\src\Release\glfw3.lib
//     opengl32.lib
//     glu32.lib
// Add Reference glfw


//#cmake:add-file f450-common-display.h

#include "f450-common-display.h"

void check_option_from_max_to_min_and_back_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map,
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option opt, int number_of_iterations, float tolerance);
void check_both_sensors_option_from_max_to_min_and_back_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map,
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance);

TEST_CASE("Exposure - each sensor and both - no streaming", "[F450]") {
    using namespace std;

    //------------------
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;

    //------------

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
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_Y16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_Y16);

    rs2::pipeline pipe;
    pipe.start(cfg);

    std::cout << "pipe started" << std::endl;
    {
        rs2::sensor sensor = devices[0].first<rs2::fa_infrared_sensor>();

        int number_of_iterations = 5;
        float tolerance = 3; // for 5% - not absolute as done in gain

        //set AUTO EXPOSURE to manual
        sensor.set_option(RS2_OPTION_AUTO_EXPOSURE_MODE, 0);
        REQUIRE(sensor.get_option(RS2_OPTION_AUTO_EXPOSURE_MODE) == 0);

        rs2_option opt = RS2_OPTION_GAIN;

        check_option_from_max_to_min_and_back_streaming(app, printer, color_map, pipe, sensor, RS2_OPTION_EXPOSURE, number_of_iterations, tolerance);
        //--------------------------------------------------------------------------
        check_option_from_max_to_min_and_back_streaming(app, printer, color_map, pipe, sensor, RS2_OPTION_EXPOSURE_SECOND, number_of_iterations, tolerance);
        //--------------------------------------------------------------------------
        check_both_sensors_option_from_max_to_min_and_back_streaming(app, printer, color_map, pipe, sensor, RS2_OPTION_EXPOSURE, RS2_OPTION_EXPOSURE_SECOND, number_of_iterations, tolerance);
    }
    pipe.stop(); 
    std::cout << "pipe stopped" << std::endl;
}


void check_option_from_max_to_min_and_back_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map,
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option opt, int number_of_iterations, float tolerance)
{
    auto range = sensor.get_option_range(opt);
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << std::endl;

    rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    //setting exposure to max
    float valueToSet = range.max;
    sensor.set_option(opt, valueToSet);
    float valueAfterChange = sensor.get_option(opt);
    int iterations = 5;
    while (app && iterations-- && valueAfterChange != valueToSet)
    {
        rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        app.show(data);
        valueAfterChange = sensor.get_option(opt);
    }
    REQUIRE(valueAfterChange == valueToSet);
    std::cout << std::endl;
    std::cout << "Max Value Checked" << std::endl;

    //going down dividing by 2 at each iteration until min
    checkOption_streaming(app, printer, color_map, pipe, sensor, opt, valueAfterChange, 15.0f,//range.min
 number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value /= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        }
        );

    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange = sensor.get_option(opt);
    REQUIRE(valueAfterChange == 15.0f);//range.min

    std::cout << std::endl;
    std::cout << "Option decremented and got to min value Checked" << std::endl;

    //then back to max multiplying by 2 until max
    checkOption_streaming(app, printer, color_map, pipe, sensor, opt, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value *= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        });

    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange = sensor.get_option(opt);
    REQUIRE(valueAfterChange == range.max);

    std::cout << std::endl;
    std::cout << "Option incremented and got back to max value Checked" << std::endl;
}


void check_both_sensors_option_from_max_to_min_and_back_streaming(window& app, rs2::rates_printer& printer, rs2::colorizer& color_map, 
    const rs2::pipeline& pipe, const rs2::sensor& sensor, rs2_option opt, rs2_option opt_second, int number_of_iterations, float tolerance)
{
    std::cout << "-----------------------------------------------------------" << std::endl;
    std::cout << "Checking option = " << rs2_option_to_string(opt) << " for both sensors" << std::endl;

    rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);

    auto range = sensor.get_option_range(opt);
    auto range_second = sensor.get_option_range(opt_second);
    REQUIRE(range.min == range_second.min);
    REQUIRE(range.max == range_second.max);
    REQUIRE(range.step == range_second.step);
    std::cout << "Min, Max, Step checked same for both sensors" << std::endl;


    //setting exposure to max
    float valueToSet = range.max;
    sensor.set_option(opt, valueToSet);
    sensor.set_option(opt_second, valueToSet);
    float valueAfterChange = sensor.get_option(opt);
    float valueAfterChange_second = sensor.get_option(opt_second);
    int iterations = number_of_iterations;
    while (iterations-- && valueAfterChange != valueToSet)
    {
        rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        app.show(data);
        valueAfterChange = sensor.get_option(opt);
    }
    iterations = number_of_iterations;
    while (iterations-- && valueAfterChange_second != valueToSet)
    {
        rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
        app.show(data);
        valueAfterChange_second = sensor.get_option(opt_second);
    }
    REQUIRE(valueAfterChange == valueToSet);
    REQUIRE(valueAfterChange_second == valueToSet);
    std::cout << std::endl;
    std::cout << "Max Value Checked" << std::endl;

    //going down dividing by 2 at each iteration until min
    checkOptionForBothSensors_streaming(app, printer, color_map, pipe, sensor, opt, opt_second, valueAfterChange, 15.0f, //range.min
     number_of_iterations, tolerance, [](float value, float limit) {
        return value > limit;
        },
        [&](float& value) {
            value /= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        }
        );

    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange = sensor.get_option(opt);
    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange_second = sensor.get_option(opt_second);
    REQUIRE(valueAfterChange == 15.0f); //range.min
    REQUIRE(valueAfterChange_second == 15.0f); //range.min

    std::cout << std::endl;
    std::cout << "Option decremented and got to min value Checked" << std::endl;

    //then back to max multiplying by 2 until max
    checkOptionForBothSensors_streaming(app, printer, color_map, pipe, sensor, opt, opt_second, valueAfterChange, range.max, number_of_iterations, tolerance, [](float value, float limit) {
        return value < limit;
        },
        [&](float& value) {
            value *= 2.f;
        },
            [tolerance](float valueAfterChange, float valueRequested) {
            return (valueAfterChange >= (1.f - tolerance) * valueRequested) && (valueAfterChange <= (1.f + tolerance) * valueRequested);
        }
        );

    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange = sensor.get_option(opt);
    data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
    app.show(data);
    valueAfterChange_second = sensor.get_option(opt_second);
    //REQUIRE(valueAfterChange == range.max);
    //REQUIRE(valueAfterChange_second == range.max);

    std::cout << std::endl;
    std::cout << "Option incremented and got back to max value Checked" << std::endl;
}

/*
TEST_CASE("Exposure - each sensor and both - no streaming", "[F450]") {
    using namespace std;

    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;

    //-------------------------------------------------------------------
    // Obtain a list of devices currently present on the system
    rs2::context ctx;
    rs2::device d;

    auto devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?");
        return;
    }
    // Retrieve the viable devices
    std::vector<rs2::device> devices;

    for (auto i = 0u; i < device_count; i++)
    {
        try
        {
            auto dev = devices_list[i];
            devices.emplace_back(dev);
        }
        catch (const std::exception & e)
        {
            std::cout << "Could not create device - " << e.what() << " . Check SDK logs for details" << std::endl;
        }
        catch (...)
        {
            std::cout << "Failed to created device. Check SDK logs for details" << std::endl;
        }
    }


    //-------------------------------------------------------------------
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_Y16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_Y16);
    //cfg.enable_stream(RS2_STREAM_INFRARED, 2, RS2_FORMAT_RGB8);// , RS2_FORMAT_Y16, 5);
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Start streaming with default recommended configuration
    // The default video configuration contains Depth and Color streams
    // If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default
    pipe.start(cfg);

    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames().    // Wait for next set of frames from the camera
            apply_filter(printer).     // Print each enabled stream frame rate
            apply_filter(color_map);   // Find and colorize the depth data

// The show method, when applied on frameset, break it to frames and upload each frame into a gl textures
// Each texture is displayed on different viewport according to it's stream unique id
        app.show(data);

        
    }

}*/
