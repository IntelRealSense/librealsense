// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering
#include "labeled-pointcloud-drawing.hpp"

#include <algorithm>            // std::min, std::max
#include <set>

// Helper functions
void register_glfw_callbacks(window& app, glfw_state& app_state);

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Pointcloud Example");
    // Construct an object to manage view state
    glfw_state app_state;
    app_state.offset_y = 90;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;

    // Defining a streaming configuration with the labeled point cloud streaming
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_LABELED_POINT_CLOUD);
    
    // Start streaming with this configuration
    pipe.start(cfg);

    while (app) // Application still alive?
    {
        // Wait for the next set of frames from the camera
        auto data = pipe.wait_for_frames();

        auto labels_frame = data.get_labeled_point_cloud_frame();

        auto vertices = labels_frame.get_vertices();
        auto vertices_size = labels_frame.size();

        std::vector<rs2::vertex> vertices_vec;
        vertices_vec.insert(vertices_vec.begin(), vertices, vertices + vertices_size);

        auto labels = labels_frame.get_labels();

        std::vector<uint8_t> labels_vec;
        labels_vec.insert(labels_vec.begin(), labels, labels + vertices_size);

        // Draw the labeled pointcloud
        draw_labeled_pointcloud(app.width(), app.height(), app_state, vertices_vec, labels_vec);
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

