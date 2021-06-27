// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <fstream>              // File IO
#include <iostream>             // Terminal IO
#include <sstream>              // Stringstreams

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// Helper function for writing metadata to disk as a csv file
void metadata_to_csv(const rs2::frame& frm, const std::string& filename);

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display.
int main(int argc, char * argv[]) try
{
    int capture_times = 1;
    if (argc == 2) {
	    std::cout << " === " << argv[1] << std::endl;
	    capture_times = std::atoi(argv[1]);
    }

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

    // Declare RealSense pipeline, encapsulating the actual device and sensors
    rs2::pipeline pipe;
    // Start streaming with default recommended configuration
    pipe.start();

    // Capture 30 frames to give autoexposure, etc. a chance to settle
    for (auto i = 0; i < 30; ++i) pipe.wait_for_frames();

    	for (size_t times = 1; times <= capture_times; ++times) {
    		// Wait for the next set of frames from the camera. Now that autoexposure, etc.
    		// has settled, we will write these to disk
    		for (auto&& frame : pipe.wait_for_frames())
    		{
        		// We can only save video frames as pngs, so we skip the rest
        		if (auto vf = frame.as<rs2::video_frame>())
        		{
            		auto stream = frame.get_profile().stream_type();
            		// Use the colorizer to get an rgb image for the depth stream
            		if (vf.is<rs2::depth_frame>()) vf = color_map.process(frame);

            		// Write images to disk
            		std::stringstream png_file;
            		png_file << "rs-save-to-disk-output-" << vf.get_profile().stream_name() << "_" << times << ".png";
            		stbi_write_png(png_file.str().c_str(), vf.get_width(), vf.get_height(),
                           		vf.get_bytes_per_pixel(), vf.get_data(), vf.get_stride_in_bytes());
            		std::cout << "Saved " << png_file.str() << std::endl;  		
        		}
    		}
    	}

    return EXIT_SUCCESS;
}
catch(const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch(const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

