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
#include <map>
#include <limits>
#include <iostream>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

void normalize_depth_to_rgb(uint8_t rgb_image[], const uint16_t depth_image[], int width, int height)
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

std::map<rs::stream,int> components_map =
{
    { rs::stream::depth,     3  },      // RGB
    { rs::stream::color,     3  },
    { rs::stream::infrared , 1  },      // Monochromatic
    { rs::stream::infrared2, 1  },
    { rs::stream::fisheye,   1  }
};

struct stream_record
{
    stream_record(void): frame_data(nullptr) {};
    stream_record(rs::stream value): stream(value), frame_data(nullptr) {};
    ~stream_record() { frame_data = nullptr;}
    rs::stream          stream;
    rs::intrinsics      intrinsics;
    unsigned char   *   frame_data;
};


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

    std::vector<stream_record> supported_streams;

    for (int i=(int)rs::capabilities::depth; i <=(int)rs::capabilities::fish_eye; i++)
        if (dev->supports((rs::capabilities)i))
            supported_streams.push_back(stream_record((rs::stream)i));

    for (auto & stream_record : supported_streams)
        dev->enable_stream(stream_record.stream, rs::preset::best_quality);

    /* activate video streaming */
    dev->start();

    /* retrieve actual frame size for each enabled stream*/
    for (auto & stream_record : supported_streams)
        stream_record.intrinsics = dev->get_stream_intrinsics(stream_record.stream);

    /* Capture 30 frames to give autoexposure, etc. a chance to settle */
    for (int i = 0; i < 30; ++i) dev->wait_for_frames();

    /* Retrieve data from all the enabled streams */
    for (auto & stream_record : supported_streams)
        stream_record.frame_data = const_cast<uint8_t *>((const uint8_t*)dev->get_frame_data(stream_record.stream));

    /* Transform Depth range map into color map */
    stream_record depth = supported_streams[(int)rs::stream::depth];
    std::vector<uint8_t> coloredDepth(depth.intrinsics.width * depth.intrinsics.height * components_map[depth.stream]);

    /* Encode depth data into color image */
    normalize_depth_to_rgb(coloredDepth.data(), (const uint16_t *)depth.frame_data, depth.intrinsics.width, depth.intrinsics.height);

    /* Update captured data */
    supported_streams[(int)rs::stream::depth].frame_data = coloredDepth.data();

    /* Store captured frames into current directory */
    for (auto & captured : supported_streams)
    {
        std::stringstream ss;
        ss << "cpp-headless-output-" << captured.stream << ".png";

        std::cout << "Writing " << ss.str().data() << ", " << captured.intrinsics.width << " x " << captured.intrinsics.height << " pixels"   << std::endl;

        stbi_write_png(ss.str().data(),
            captured.intrinsics.width,captured.intrinsics.height,
            components_map[captured.stream],
            captured.frame_data,
            captured.intrinsics.width * components_map[captured.stream] );
    }

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
