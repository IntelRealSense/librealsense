// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include "proc/synthetic-stream.h"
#include "proc/decimation-filter.h"
#include "proc/spatial-filter.h"

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");
    // Declare two textures on the GPU, one for color and one for depth
    texture depth_image, filtered_image;

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    //rs2::decimation_filter depth_filter;
    rs2::spatial_filter depth_filter;
    rs2::option_range rng = depth_filter.get_option_range(rs2_option::RS2_OPTION_FILTER_MAGNITUDE);
    size_t counter = 0;
    size_t cur_idx = 0;
    std::vector<float> filter_magnitudes;
    for (float v = rng.min; v <= rng.max; v += rng.step)
        filter_magnitudes.push_back(v);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    while (app) // Application still alive?
    {
        rs2::frameset data = pipe.wait_for_frames(); // Wait for next set of frames from the camera

        // Apply filter
        rs2::frameset post_processed = depth_filter.proccess(data);

        rs2::frame depth_original = color_map(data.get_depth_frame()); // Find and colorize the original depth data
        rs2::frame depth_processed = color_map(post_processed.get_depth_frame()); // Find and colorize the filtered depth data

        // Render original depth on to the first half of the screen and the filtered data on the second
        depth_image.render(     depth_original, { 0,               0, app.width() / 2, app.height() });
        filtered_image.render( depth_processed, { app.width() / 2, 0, app.width() / 2, app.height() });

        if (false && (0 ==(++counter % 40)))
        {
            auto new_val = filter_magnitudes[(++cur_idx) % filter_magnitudes.size()];
            std::cout << "New magnitude is " << new_val << std::endl;
            depth_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, new_val);
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
