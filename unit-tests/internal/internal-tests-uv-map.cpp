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
#include "./../unit-tests-common.h"
#include "./../src/environment.h"

using namespace librealsense;
using namespace librealsense::platform;

std::vector<device_profiles> uv_tests_configurations = {
    //D465
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 1024, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 2000, 1500, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280,  720, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 2000, 1500, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  960,  768, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 2000, 1500, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 1024, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 1920, 1080, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280,  720, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 1920, 1080, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  960,  768, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV, 1920, 1080, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280, 1024, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV,  960,  720, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 1280,  720, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV,  960,  720, 0 } }, 30, true },
    { { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  960,  768, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV,  960,  720, 0 } }, 30, true },
    //L500
    //{ { { RS2_STREAM_DEPTH, RS2_FORMAT_Z16,  1024,  768, 0 },{ RS2_STREAM_INFRARED, RS2_FORMAT_Y8,  1024,  768, 0 },{ RS2_STREAM_COLOR, RS2_FORMAT_YUYV,  1920,  1080, 0 } }, 30, true },
};

TEST_CASE("Generate UV-MAP", "[live]")
{
    log_to_console(rs2_log_severity::RS2_LOG_SEVERITY_WARN);

    // Require at least one device to be plugged in
    rs2::context ctx;
    auto list = ctx.query_devices();
    REQUIRE(list.size());

    for (auto&& test_cfg : uv_tests_configurations)
    {
        // Record Depth + RGB streams
        {
            // We want the points object to be persistent so we can display the last cloud when a frame drops
            rs2::points points;

            rs2::pointcloud pc;
            // Declare RealSense pipeline, encapsulating the actual device and sensors
            rs2::pipeline pipe;

            rs2::config cfg;
            for (auto&& pf: test_cfg.streams)
                cfg.enable_stream(pf.stream, pf.index, pf.width, pf.height, pf.format, pf.fps);

            // Start streaming with default recommended configuration
            auto pf = cfg.resolve(pipe);
            for (auto&& snr : pf.get_device().query_sensors())
            {
                if (snr.supports(RS2_OPTION_GLOBAL_TIME_ENABLED))
                    snr.set_option(RS2_OPTION_GLOBAL_TIME_ENABLED, false);

                if (snr.supports(RS2_OPTION_EMITTER_ENABLED))
                {
                    snr.set_option(RS2_OPTION_EMITTER_ENABLED, false);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    snr.set_option(RS2_OPTION_EMITTER_ENABLED, true);
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    snr.set_option(RS2_OPTION_LASER_POWER, 60.f);
                }
            }

            rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise
            const std::string disparity_filter_name = "Disparity";
            rs2::disparity_transform depth_to_disparity(true);
            rs2::disparity_transform disparity_to_depth(false);
            std::vector<rs2::filter> filters;

            // The following order of emplacement will dictate the orders in which filters are applied
            filters.push_back(std::move(depth_to_disparity));
            filters.push_back(std::move(temp_filter));
            filters.push_back(std::move(disparity_to_depth));

            // Start streaming with default recommended configuration
            pf = pipe.start(cfg);

            size_t i = 0;
            size_t startup = 50;

            while (i < startup + 200)
            {
                // Wait for the next set of frames from the camera
                auto frames = pipe.wait_for_frames();
                if (++i < startup)
                    continue;

                auto depth = frames.get_depth_frame();
                auto color = frames.get_color_frame();
                // For D465 streaming of Depth + IR + RGB at 4K is not possible.
                // Therefore the data collection will be performed in two iterations
                //auto ir = frames.get_infrared_frame(1);

                // Prerequisite is that the required three streams are available and that the depth/ir are aligned
                if (!(color && depth /*&& ir && (depth.get_frame_number() == ir.get_frame_number())*/))
                    return;

                // Using of the temporal filter shall be recommended
                //// Filter depth stream
                //for (auto&& filter : filters)
                //    depth = filter.process(depth);

                pc.map_to(color);
                // Generate the pointcloud and texture mappings
                points = pc.process(depth);

                auto pf = color.get_profile().as<rs2::video_stream_profile>();
                auto w = pf.width();
                auto h = pf.height();
                std::stringstream ss;
                ss << "_yuyv_" << w << "_" << h;
                std::string rgb_res(ss.str().c_str());

                ss.clear(); ss.str(""); ss << i - startup << "_yuyv_" << w << "_" << h << ".bin";
                {
                    assert(color.get_data_size() == w * h * sizeof(uint16_t));
                    std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                    outfile.write((const char*)color.get_data(), color.get_data_size());
                }

                pf = depth.get_profile().as<rs2::video_stream_profile>();
                w = pf.width();
                h = pf.height();

                auto tex_ptr = (float2*)points.get_texture_coordinates();
                auto vert_ptr = (rs2::vertex*)points.get_vertices();
                auto uv_map = reinterpret_cast<const char*>(tex_ptr);

                std::cout << "Frame size is " << w * h << std::endl;

                ss.clear(); ss.str("");  ss << i - startup << "_uv" << w << "_" << h << rgb_res <<".bin";
                {
                    std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                    outfile.write(uv_map, w * h * sizeof(float2));
                }

                pf = depth.get_profile().as<rs2::video_stream_profile>();
                w = pf.width();
                h = pf.height();
                ss.clear(); ss.str("");  ss << i - startup << "_depth_" << w << "_" << h << rgb_res << ".bin";
                {
                    REQUIRE(depth.get_data_size() == w * h * sizeof(uint16_t));
                    std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                    outfile.write((const char*)depth.get_data(), depth.get_data_size());
                }

                //pf = ir.get_profile().as<rs2::video_stream_profile>();
                //w = pf.width();
                //h = pf.height();
                //ss.clear(); ss.str("");  ss << i - startup << "_ir_" << w << "_" << h << ".bin";
                //{
                //    assert(ir.get_data_size() == w * h * sizeof(uint8_t));
                //    std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                //    outfile.write((const char*)ir.get_data(), ir.get_data_size());
                //}

                std::cout << "Iteration " << i - startup << " files were written" << std::endl;
            }
        }

        // record IR stream separately
        if (true)
        {
            rs2::pipeline pipe;
            rs2::config cfg;

            auto fmt = test_cfg.streams[0]; // Depth and IR align
            cfg.enable_stream(rs2_stream::RS2_STREAM_INFRARED, 1, fmt.width, fmt.height, RS2_FORMAT_Y8, fmt.fps);

            // Start streaming with default recommended configuration
            auto pf = cfg.resolve(pipe);
            for (auto&& snr : pf.get_device().query_sensors())
            {
                if (snr.supports(RS2_OPTION_EMITTER_ENABLED))
                    snr.set_option(RS2_OPTION_EMITTER_ENABLED, false);
            }

            // Start streaming with default recommended configuration
            pf = pipe.start(cfg);

            size_t i = 0;
            size_t startup = 30;

            while (i < startup)
            {
                // Wait for the next set of frames from the camera
                auto frames = pipe.wait_for_frames();
                if (++i < startup)
                    continue;

                auto ir_frame = frames.get_infrared_frame(1);

                // Prerequisite is that the required three streams are available and that the depth/ir are aligned
                if (!ir_frame)
                    return;

                auto pf = ir_frame.get_profile().as<rs2::video_stream_profile>();
                auto w = pf.width();
                auto h = pf.height();
                std::stringstream ss;
                ss << i - startup << "_ir_" << w << "_" << h << ".bin";
                {
                    assert(ir_frame.get_data_size() == w * h * sizeof(uint16_t));
                    std::ofstream outfile(ss.str().c_str(), std::ios::binary);
                    outfile.write((const char*)ir_frame.get_data(), ir_frame.get_data_size());
                }

                std::cout << "Iteration " << i - startup << " files were written" << std::endl;
            }
        }
    }
}
