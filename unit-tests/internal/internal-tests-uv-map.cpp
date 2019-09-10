// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include "catch/catch.hpp"
#include <cmath>
#include <iostream>
#include <chrono>
#include <ctime>
#include <algorithm>
#include <librealsense2/rs.hpp>
#include <librealsense2/hpp/rs_sensor.hpp>
#include "../../common/tiny-profiler.h"
#include "./../src/environment.h"

using namespace librealsense;
using namespace librealsense::platform;


TEST_CASE("Generate UV-MAP", "[live]")
{
    // Require at least one device to be plugged in
    rs2::context ctx;
    {
        auto list = ctx.query_devices();
        REQUIRE(list.size());

        //std::cout << " Initial Extrinsic Graph size is " << init_size << std::endl;

        rs2::pointcloud pc;
        // We want the points object to be persistent so we can display the last cloud when a frame drops
        rs2::points points;

        // Declare RealSense pipeline, encapsulating the actual device and sensors
        rs2::pipeline pipe;

        rs2::config cfg;
        cfg.enable_stream(rs2_stream::RS2_STREAM_DEPTH,0, 1280, 720, RS2_FORMAT_Z16,15);
        cfg.enable_stream(rs2_stream::RS2_STREAM_INFRARED,1);
        cfg.enable_stream(rs2_stream::RS2_STREAM_COLOR, 0, 2000, 1500, RS2_FORMAT_YUYV, 15);

        // Start streaming with default recommended configuration
        auto pf = pipe.start(cfg);

        auto i = 0UL;

        while (i<100) // Application still alive?
        {
            // Wait for the next set of frames from the camera
            auto frames = pipe.wait_for_frames();
            if (++i < 30)
                return;

            auto depth = frames.get_depth_frame();
            auto color = frames.get_color_frame();
            auto ir = frames.get_infrared_frame(1);

            // Prerequisite is that the required three streams are available and that the depth/ir are aligned
            if (!(color && depth && ir && (depth.get_frame_number() == ir.get_frame_number())))
                return;

            // Tell pointcloud object to map to this color frame
            pc.map_to(color);

            // Generate the pointcloud and texture mappings
            points = pc.process(depth);

            auto pf = depth.get_profile().as<rs2::video_stream_profile>();
            auto w = pf.width();
            auto h = pf.height();

            auto tex_ptr = (float2*)points.get_texture_coordinates();
            auto vert_ptr = (rs2::vertex*)points.get_vertices();
            auto uv_map = reinterpret_cast<const char*>(tex_ptr);

            std::cout << "Frame size is " << w * h << std::endl;
            std::stringstream ss;
            ss << "uv_" << i << ".bin";

            {
                std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                outfile.write(uv_map, w * h * sizeof(float2));
            }

            ss.clear();  ss << "ir_" << i << ".bin";
            {
                assert(color.get_data_size() == w * h * sizeof(uint16_t));
                std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                outfile.write((const char*)ir.get_data(), ir.get_data_size());
            }

            ss.clear();  ss << "uyuv_" << i << ".bin";
            {
                assert(color.get_data_size() == w * h * sizeof(uint16_t));
                std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                outfile.write((const char*)color.get_data(), color.get_data_size());
            }

            std::cout << "Iteration " << i << " files were written" << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }
}
