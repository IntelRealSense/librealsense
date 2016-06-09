// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <thread>
#include <iostream>

int main() try
{
    rs::context ctx;
    if(ctx.get_device_count() == 0) return EXIT_FAILURE;
    rs::device * dev = ctx.get_device(0);

    dev->enable_stream(rs::stream::depth, rs::preset::best_quality);
    dev->enable_stream(rs::stream::color, rs::preset::best_quality);

    // rs::device is NOT designed to be thread safe, so you should not currently make calls
    // against it from your callback thread. If you need access to information like 
    // intrinsics/extrinsics, formats, etc., capture them ahead of time
    rs::intrinsics depth_intrin, color_intrin;
    rs::format depth_format, color_format;
    depth_intrin = dev->get_stream_intrinsics(rs::stream::depth);
    depth_format = dev->get_stream_format(rs::stream::depth);
    color_intrin = dev->get_stream_intrinsics(rs::stream::color);
    color_format = dev->get_stream_format(rs::stream::color);

    // Set callbacks prior to calling start()
    auto depth_callback = [depth_intrin, depth_format](rs::frame f)
    {
        std::cout << depth_intrin.width << "x" << depth_intrin.height
            << " " << depth_format << "\tat t = " << f.get_timestamp() << " ms" << std::endl;
    };
    auto color_callback = [color_intrin, color_format](rs::frame f)
    {
        std::cout << color_intrin.width << "x" << color_intrin.height
            << " " << color_format << "\tat t = " << f.get_timestamp() << " ms" << std::endl;
    };

    dev->set_frame_callback(rs::stream::depth, depth_callback);
    dev->set_frame_callback(rs::stream::color, color_callback);

    // Between start() and stop(), you will receive calls to the specified callbacks from background threads
    dev->start();
    std::this_thread::sleep_for(std::chrono::seconds(10));
    // NOTE: No need to call wait_for_frames() or poll_for_frames(), though they should still work as designed
    dev->stop();

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
