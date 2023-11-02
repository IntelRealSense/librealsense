// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API

#include <rsutils/time/timer.h>
#include <iostream>             // for cout
#include <chrono>
#include <thread>

int main(int argc, char * argv[]) try
{
    rs2::log_to_console( RS2_LOG_SEVERITY_DEBUG );

    rs2::context context;
    auto devices = context.query_devices();
    if( devices.size() )
    {
        // This can run under GHA, too -- so we don't always have devices
        // Run for just a little bit, count the frames
        size_t n_frames = 0;
        {
            rs2::pipeline p( context );
            auto profile = p.start();
            auto device = profile.get_device();
            std::cout << "Streaming on " << device.get_info( RS2_CAMERA_INFO_NAME ) << std::endl;

            rsutils::time::timer timer( std::chrono::seconds( 3 ) );
            while( ! timer.has_expired() )
            {
                // Block program until frames arrive
                rs2::frameset frames = p.wait_for_frames();
                ++n_frames;
            }
        }

        std::cout << "Got " << n_frames << " frames" << std::endl;
    }
    else
    {
        // Allow enough time for DDS enumeration
        std::this_thread::sleep_for( std::chrono::seconds( 3 ) );
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
