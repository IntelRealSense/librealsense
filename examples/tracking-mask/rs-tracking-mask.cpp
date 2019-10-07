// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2019 Intel Corporation. All Rights Reserved.
#include <librealsense2/rs.hpp>
#include <librealsense2/rsutil.h>
#include <array>
#include <cmath>
#include <iostream>
#include <vector>
#include "example.hpp"
#include <string.h> // memset
#include <memory>

std::unique_ptr<uint8_t[]> mask_image(rs2::video_frame & frame, uint8_t * mask)
{
    uint8_t * frame_data = (uint8_t *) frame.get_data();
    int width = frame.get_width();
    int height = frame.get_height();
    int mask_width = width / 8;
    std::unique_ptr<uint8_t []> output(new uint8_t[width*height]);
    // Iterate over each pixel in the image and check the mask to see
    // if the pixel is visible to the tracking algorithm or not.
    for(int y = 0; y < height; y++) {
        int my = y / 8;
        for(int x = 0; x < width; x++) {
            int mx = x / 8;
            if(mask[my*mask_width + mx])
                output[y * width + x] = frame_data[y * width + x];
            else
                output[y * width + x] = 0;
        }
    }
    return output;
}

int main(int argc, char * argv[]) try
{
    std::cout << "Waiting for device..." << std::endl;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Create a configuration for configuring the pipeline with a non default profile
    rs2::config cfg;
    // Enable fisheye streams
    cfg.enable_stream(RS2_STREAM_FISHEYE, 1);
    cfg.enable_stream(RS2_STREAM_FISHEYE, 2);
    // Start pipeline with chosen configuration
    rs2::pipeline_profile pipe_profile = pipe.start(cfg);

    // T265 has two fisheye sensors, we can choose any of them (index 1 or 2)
    const int fisheye_sensor_idx = 1;

    // Get fisheye sensor intrinsics parameters
    rs2::stream_profile fisheye_stream = pipe_profile.get_stream(RS2_STREAM_FISHEYE, fisheye_sensor_idx);
    rs2_intrinsics intrinsics = fisheye_stream.as<rs2::video_stream_profile>().get_intrinsics();
    auto tm2 = pipe_profile.get_device().as<rs2::tm2>();

    // Create an OpenGL display window and a texture to draw the masked fisheye image
    window app(intrinsics.width, intrinsics.height, "Intel RealSense T265 Tracking Mask Example");
    window_key_listener key_watcher(app);
    texture fisheye_image;

    bool mask_set = false;
    // Main loop
    while (app)
    {
        {
            // Wait for the next set of frames from the camera
            auto frames = pipe.wait_for_frames();
            // Get a frame from the fisheye stream
            rs2::video_frame fisheye_frame = frames.get_fisheye_frame(fisheye_sensor_idx);

            if(!mask_set) {
                int width = intrinsics.width/8;
                int height = intrinsics.height/8;
                auto mask_ptr = std::unique_ptr<uint8_t []>(new uint8_t[width*height]);
                double global_ts_ms = fisheye_frame.get_timestamp();
                // By default, mask out everything
                memset(mask_ptr.get(), 0, width*height);
                // Allow tracking on a 60 wide by 20 high region of
                // the image. Note that these are in mask coordinates,
                // so they correspond to a 60*8 = 480 wide by 20*8 =
                // 160 high region of the fisheye image.
                for(int y = 30; y < 50; y++) {
                    for(int x = 40; x < 100; x++) {
                        mask_ptr[y*width + x] = 255;
                    }
                }
                tm2.set_tracking_mask(fisheye_sensor_idx, mask_ptr.get(), width, height, global_ts_ms);
                mask_set = true;
            }

            uint8_t * mask = nullptr;
            int width, height;
            double global_ts_ms;
            tm2.get_tracking_mask(fisheye_sensor_idx, &mask, &width, &height, &global_ts_ms);

            if(mask) {
                // Render the fisheye image with a mask
                auto image = mask_image(fisheye_frame, mask);
                fisheye_image.render(image.get(), RS2_FORMAT_Y8, fisheye_frame.get_width(), fisheye_frame.get_height(), {0, 0, app.width(), app.height()}, 1);
                free(mask); // alloced by get_tracking_mask
            }
            else {
                // No mask has been set, or the mask we have set has
                // not yet taken effect. Render the full frame.
                fisheye_image.render(fisheye_frame.get_data(), RS2_FORMAT_Y8, fisheye_frame.get_width(), fisheye_frame.get_height(), {0, 0, app.width(), app.height()}, 1);
            }

            // By closing the current scope we let frames be deallocated, so we do not fill up librealsense queues while we do other computation.
        }

        // Check if a key is pressed
        switch (key_watcher.get_key())
        {
        case GLFW_KEY_ESCAPE:
            // Exit if user presses escape
            app.close();
            break;
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
