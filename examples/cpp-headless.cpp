// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////
// cpp-headless  //
///////////////////

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display. 

#include <librealsense/rs.hpp>

#include <cstdio>
#include <stdint.h>
#include <vector>
#include <limits>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

void normalize_depth_to_rgb(uint8_t rgb_image[640*480*3], const uint16_t depth_image[], int width, int height)
{
    for (int i = 0; i < width * height; ++i)
    {
        if (auto d = depth_image[i])
        {
            uint8_t v = d * 255 / std::numeric_limits<uint16_t>::max();
            rgb_image[i*3 + 0] = 255 - v;
            rgb_image[i*3 + 1] = 255 - v;
            rgb_image[i*3 + 2] = 255 - v;
        }
        else
        {
            rgb_image[i*3 + 0] = 0;
            rgb_image[i*3 + 1] = 0;
            rgb_image[i*3 + 2] = 0;
        }
    }
}

int main() try
{
    rs::log_to_console(rs::log_severity::warn);
    //rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // Configure depth to run at VGA resolution at 30 frames per second
    dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
    dev->enable_stream(rs::stream::color, 640, 480, rs::format::rgb8, 30);
    dev->enable_stream(rs::stream::infrared, 640, 480, rs::format::y8, 30);

    const int width = 640;
    const int height = 480;

    dev->start();

    // Capture 30 frames to give autoexposure, etc. a chance to settle
    for (int i = 0; i < 30; ++i) dev->wait_for_frames();

    // Retrieve depth data, which was previously configured as a 640 x 480 image of 16-bit depth values
    const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));
    const uint8_t * color_frame = reinterpret_cast<const uint8_t *>(dev->get_frame_data(rs::stream::color));
    const uint8_t * ir_frame = reinterpret_cast<const uint8_t *>(dev->get_frame_data(rs::stream::infrared));

    std::vector<uint8_t> coloredDepth(width * height * 3);
    normalize_depth_to_rgb(coloredDepth.data(), depth_frame, width, height);
    stbi_write_png("cpp-headless-output-depth.png", width, height, 3, coloredDepth.data(), 3 * width);
    stbi_write_png("cpp-headless-output-rgb.png", width, height, 3, color_frame, 3 * width);
    stbi_write_png("cpp-headless-output-ir.png", width, height, 1, ir_frame, width);

    printf("wrote frames to current working directory.\n");
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
