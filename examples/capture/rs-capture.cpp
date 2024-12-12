// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <iostream>
#include <rsutils/string/from.h>

std::ostream & operator<<( std::ostream & ss, const rs2::frame & self )
{
    ss << rs2_format_to_string( self.get_profile().format() );
    ss << " #" << self.get_frame_number();
    ss << " @" << rsutils::string::from( self.get_timestamp() );
    return ss;
}

std::string print_frameset(rs2::frame f)
{
    std::ostringstream ss;
    ss << "frame";
    if (auto fs = f.as<rs2::frameset>())
    {
        ss << "set";
        for(auto sf : fs)
        {
            ss << " " << sf;
        }
    }
    else
    {
        ss << " " << f;
    }
    return ss.str();
}

void run_and_verify_frame_received(rs2::pipeline& pipe)
{
    auto start = std::chrono::high_resolution_clock().now(); 
    pipe.start();
    auto frameset = pipe.wait_for_frames();
    auto stop = std::chrono::high_resolution_clock().now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    /*auto depth_frame = frameset.get_depth_frame();
    if( depth_frame)
    {
        std::cout << "D";
    }
    auto color_frame = frameset.get_color_frame();
    if( color_frame)
    {
        std::cout << "C";
    }
    std::cout << std::endl;*/
    std::ostringstream ss;
    ss << "After " << duration_ms << "[msec], got first frame of " << print_frameset(frameset);
    std::cout << ss.str() << std::endl;
    pipe.stop();
}

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_DEBUG, "mylog.txt");

    auto ctx = rs2::context();
    auto dev = ctx.query_devices().front();
    rs2::pipeline pipe(ctx);
    pipe.set_device(&dev);

    int iterations = 0;
    while (++iterations < 500) // Application still alive?
    {
        std::cout << "iteration no" << iterations << std::endl;
        run_and_verify_frame_received(pipe);
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

