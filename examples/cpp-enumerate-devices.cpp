// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>
#include <iomanip>

int main() try
{
    rs::context ctx;
    auto list = ctx.query_devices();

    for (auto& info : list)
    {
        auto dev = ctx.create(info);

        auto& color = dev.color();
        auto color_formats = color.get_stream_profiles();

        auto& depth = dev.depth();
        auto depth_formats = depth.get_stream_profiles();

    }

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
