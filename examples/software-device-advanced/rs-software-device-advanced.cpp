/* License: Apache 2.0. See LICENSE file in root directory.
Copyright(c) 2020 Intel Corporation. All Rights Reserved. */

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2/hpp/rs_internal.hpp>

#include "legacy_adaptor.hpp"
#include "example.hpp"

int main(int argc, char * argv[]) try
{
    rs2::log_to_console(RS2_LOG_SEVERITY_ERROR);
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Advanced Software Device Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;
    
    // Get software device from legacy adaptor for use with modern RealSense SDK2.0
    rs2::software_device dev = rs2::legacy::legacy_device(0);

    // Declare RealSense context to bind the software device to.
    rs2::context ctx;
    dev.add_to(ctx);

    // Declare RealSense pipeline, for ease of streaming
    rs2::pipeline pipe(ctx);

    // Start streaming with default recommended configuration
    // The default video configuration contains Depth and Color streams
    // If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default
    //rs2::frame_queue queue(5);
    //pipe.start([&](rs2::frame f) { 
    //    std::cout << "[pipe.start(<lambda>)] " << f.get_profile().stream_name() << " " << f.get_frame_number() << " " << f.get_timestamp() << std::endl; queue.enqueue(f);
    //});

    pipe.start();
    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames().    // Wait for next set of frames from the camera
            apply_filter(printer).     // Print each enabled stream frame rate
            apply_filter(color_map);   // Find and colorize the depth data

// The show method, when applied on frameset, break it to frames and upload each frame into a gl textures
// Each texture is displayed on different viewport according to it's stream unique id
        app.show(data);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;

    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}