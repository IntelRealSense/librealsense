// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include "example.hpp"          // Include short list of convenience functions for rendering

// Capture Example demonstrates how to
// capture depth and color video streams and render them to the screen
int main(int argc, char * argv[]) try
{
    rs2::context ctx;

    rs2::device_list devices = ctx.query_devices();
    rs2::device dev = devices.front();

    rs2::depth_sensor depth_sensor= dev.query_sensors().front();


    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_SIZE, 2);
    depth_sensor.set_option(RS2_OPTION_HDR_RELATIVE_MODE, 0);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 1);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 120.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 90.f);


    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 2);
    depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1200.f);
    depth_sensor.set_option(RS2_OPTION_GAIN, 20.f);

    depth_sensor.set_option(RS2_OPTION_HDR_SEQUENCE_ID, 0);

    depth_sensor.set_option(RS2_OPTION_HDR_ENABLED, 1);


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