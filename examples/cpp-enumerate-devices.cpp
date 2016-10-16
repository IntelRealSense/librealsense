// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

using namespace rs;

int main() try
{
    context ctx;
    auto list = ctx.query_devices();

    for (auto& info : list)
    {
        auto dev = ctx.create(info);

        for (auto i = 0; i < RS_CAMERA_INFO_COUNT; i++)
        {
            auto camera_info = static_cast<rs_camera_info>(i);
            if (dev.supports(camera_info))
            {
                std::cout << rs_camera_info_to_string(camera_info) 
                          << ": " << dev.get_camera_info(camera_info) << std::endl;
            }
        }

        auto& color = dev.color();
        auto color_formats = color.get_stream_profiles();
        for (auto format : color_formats)
        {
            std::cout << format.stream << " " << format.format << " " << format.width << "x" << format.height << " " << format.fps << std::endl;

            auto exp = dev.color().get_option(RS_OPTION_COLOR_EXPOSURE);
            auto mvr = dev.depth().get_option(RS_OPTION_F200_MOTION_RANGE);

            std::cout << "starting streaming " << std::endl;
            auto streaming = dev.color().open(format);

            streaming.play([](frame f)
            {
                std::cout << f.get_format() << " " << f.get_stream_type() << std::endl;
            });

            std::this_thread::sleep_for(std::chrono::seconds(10));

            streaming.stop();
        }

        auto& depth = dev.depth();
        auto depth_formats = depth.get_stream_profiles();
        for (auto format : depth_formats)
        {
            std::cout << format.stream << " " << format.format << " " << format.width << "x" << format.height << " " << format.fps << std::endl;
        }
    }

    return EXIT_SUCCESS;
}
catch(const error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
