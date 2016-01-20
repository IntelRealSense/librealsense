//  INTEL CORPORATION PROPRIETARY INFORMATION This software is supplied under the
//  terms of a license agreement or nondisclosure agreement with Intel Corporation
//  and may not be copied or disclosed except in accordance with the terms of that
//  agreement.
//  Copyright(c) 2015 Intel Corporation. All Rights Reserved.

///////////////////
// cpp-headless  //
///////////////////

// This file captures 30 frames and writes a .png file of the 29th frame to disk. It can be 
// useful for debugging an embedded system with no display. 

#include <librealsense/rs.hpp>
#include "example.hpp"

#include <cstdio>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb_image_write.h"

int main() try
{
    rs::context ctx;
    printf("There are %d connected RealSense devices.\n", ctx.get_device_count());
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;

    rs::device * dev = ctx.get_device(0);
    printf("\nUsing device 0, an %s\n", dev->get_name());
    printf("    Serial number: %s\n", dev->get_serial());
    printf("    Firmware version: %s\n", dev->get_firmware_version());

    // Configure depth to run at VGA resolution at 30 frames per second
    dev->enable_stream(rs::stream::depth, 640, 480, rs::format::z16, 30);
    dev->start();

	const rs::intrinsics depth_intrin = dev->get_stream_intrinsics(rs::stream::depth);

	const int numFramesToCap = 30;

	std::vector<uint8_t> coloredDepthHistogram;
	coloredDepthHistogram.resize(depth_intrin.width * depth_intrin.height * 3);
        
	for (int i = 0; i < numFramesToCap; ++i)
    {
        dev->wait_for_frames();

        // Retrieve depth data, which was previously configured as a 640 x 480 image of 16-bit depth values
        const uint16_t * depth_frame = reinterpret_cast<const uint16_t *>(dev->get_frame_data(rs::stream::depth));

		if (i == numFramesToCap - 1)
		{
			make_depth_histogram(coloredDepthHistogram.data(), reinterpret_cast<const uint16_t *>(depth_frame), depth_intrin.width, depth_intrin.height);
			stbi_write_png("cpp-headless-output.png", depth_intrin.width, depth_intrin.height, 3, coloredDepthHistogram.data(), 3 * depth_intrin.width * depth_intrin.height) != 0;
			printf("wrote frame to current working directory."));
		}

    }
    
    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    // Method calls against librealsense objects may throw exceptions of type rs::error
    printf("rs::error was thrown when calling %s(%s):\n", e.get_failed_function(), e.get_failed_args());
    printf("    %s\n", e.what());
    return EXIT_FAILURE;
}
