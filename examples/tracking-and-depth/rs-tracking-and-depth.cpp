// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

#include <algorithm>            // std::min, std::max
#include <fstream>              // std::ifstream

// Helper functions
void register_glfw_callbacks(window& app, glfw_state& app_state);

float detR(float H[16]) {
    return H[0]*(H[5]*H[10]-H[9]*H[6]) - H[4]*(H[1]*H[10]-H[2]*H[9]) + H[8]*(H[1]*H[6]-H[5]*H[2]);
}

int main(int argc, char * argv[]) try
{
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Tracking and Depth Example");
    // Construct an object to manage view state
    glfw_state app_state;
    // register callbacks to allow manipulation of the pointcloud
    register_glfw_callbacks(app, app_state);

    // Declare pointcloud object, for calculating pointclouds and texture mappings
    rs2::pointcloud pc;
    // We want the points object to be persistent so we can display the last cloud when a frame drops
    rs2::points points;
    // store pose and timestamp
    rs2::pose_frame pose_frame(nullptr);
    std::vector<rs2_vector> trajectory;

    rs2::context                ctx;            // Create librealsense context for managing devices
    std::vector<rs2::pipeline>  pipelines;

    // Start a streaming pipe per each connected device
    for (auto&& dev : ctx.query_devices())
    {
        rs2::pipeline pipe(ctx);
        rs2::config cfg;
        cfg.enable_device(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        pipe.start(cfg);
        pipelines.emplace_back(pipe);
    }

    // extrinsics
    // depth w.r.t. tracking (column-major)
    float H_t265_d400[16] =  {1, 0, 0, 0,
                              0,-1, 0, 0,
                              0, 0,-1, 0,
                              0, 0, 0, 1};
    std::string fn = "./H_t265_d400.cfg";
    std::ifstream ifs(fn);
    if (!ifs.is_open()) {
        std::cerr << "Couldn't open " << fn << std::endl;
        return -1;
    }
    else {
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                ifs >> H_t265_d400[i+4*j];  // row-major to column-major
            }
        }
    }
    float det = detR(H_t265_d400);
    if (fabs(1-det) > 1e-6) {
        std::cerr << "Invalid homogeneous transformation matrix input (det != 1)" << std::endl;
        return -1;
    }

    while (app) // Application still alive?
    {
        for (auto &&pipe : pipelines) // loop over pipelines
        {
            // Wait for the next set of frames from the camera
            auto frames = pipe.wait_for_frames();


            auto color = frames.get_color_frame();

            // For cameras that don't have RGB sensor, we'll map the pointcloud to infrared instead of color
            if (!color)
                color = frames.get_infrared_frame();

            // Tell pointcloud object to map to this color frame
            if (color)
                pc.map_to(color);

            auto depth = frames.get_depth_frame();

            // Generate the pointcloud and texture mappings
            if (depth)
                points = pc.calculate(depth);

            // Upload the color frame to OpenGL
            if (color)
                app_state.tex.upload(color);


            // pose
            auto pose = frames.get_pose_frame();
            if (pose) {
                pose_frame = pose;

                // Print the x, y, z values of the translation, relative to initial position
                auto pose_data = pose.get_pose_data();
                std::cout << "\r" << "Device Position: " << std::setprecision(3) << std::fixed << pose_data.translation.x << " " << pose_data.translation.y << " " << pose_data.translation.z << " (meters)";

                // add new point in the trajectory (if motion large enough to reduce size of traj. vector)
                if (trajectory.size() == 0)
                    trajectory.push_back(pose_data.translation);
                else {
                    rs2_vector prev = trajectory.back();
                    rs2_vector curr = pose_data.translation;
                    if (sqrt(pow((curr.x - prev.x), 2) + pow((curr.y - prev.y), 2) + pow((curr.z - prev.z), 2)) > 0.002)
                        trajectory.push_back(pose_data.translation);
                }
             }
        }

        // Draw the pointcloud
        if (points && pose_frame) {
            rs2_pose pose =  pose_frame.get_pose_data();
            draw_pointcloud_wrt_world(app.width(), app.height(), app_state, points, pose, H_t265_d400, trajectory);
        }
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
