// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include <proc/merge-filter.h>

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::context ctx;

    rs2::device_list devices_list = ctx.query_devices();

    size_t device_count = devices_list.size();
    if (!device_count)
    {
        std::cout << "No device detected. Is it plugged in?\n";
        return EXIT_SUCCESS;
    }

    rs2::device dev = devices_list[0];

    rs2::depth_sensor depth_sensor= dev.query_sensors().front();

    depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
    
    
    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_SIZE, 2);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 1);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);


    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 2);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 8500.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 16.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 0);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);

    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Start streaming with default recommended configuration
    // The default video configuration contains Depth and Color streams
    // If a device is capable to stream IMU data, both Gyro and Accelerometer are enabled by default
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_DEPTH);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1);
    pipe.start(cfg);

    rs2::merge_filter merging_processor;

    bool is_merge_required = false;

    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames() .    // Wait for next set of frames from the camera
            apply_filter(printer).     // Print each enabled stream frame rate
            apply_filter(color_map);   // Find and colorize the depth data

        auto depth_frame = data.get_depth_frame();
        auto hdr_seq_size = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_SIZE);
        auto hdr_seq_id = depth_frame.get_frame_metadata(RS2_FRAME_METADATA_HDR_SEQUENCE_ID);

        std::cout << "frame hdr metadata: hdr seq size = " << hdr_seq_size << ",  hdr seq id = " << hdr_seq_id << std::endl;
// The show method, when applied on frameset, break it to frames and upload each frame into a gl textures
// Each texture is displayed on different viewport according to it's stream unique id
        if (is_merge_required)
        {
            auto merged_frameset = merging_processor.process(data);
            app.show(merged_frameset);
        }
        else
        {
            app.show(data);
        }

        if (GetKeyState(VK_NUMPAD4) & 0x8000)
        {
            is_merge_required = true;
        }
        if (GetKeyState(VK_NUMPAD5) & 0x8000)
        {
            is_merge_required = false;
        }
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