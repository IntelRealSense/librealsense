// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <iostream>
#include <iomanip>

int main() try
{
    rs::log_to_console(rs::log_severity::warn);
    // rs::log_to_file(rs::log_severity::debug, "librealsense.log");

    // Obtain a list of devices currently present on the system
    rs::context ctx;
    int device_count = ctx.get_device_count();
    if(!device_count) printf("No device detected. Is it plugged in?\n");

    for(int i = 0; i < device_count; ++i)
    {
        // Show the device name and information
        rs::device * dev = ctx.get_device(i);
        std::cout << "Device " << i << " - " << dev->get_name() << ":\n";
        std::cout << " Serial number: " << dev->get_serial() << "\n";
        std::cout << " Firmware version: " << dev->get_firmware_version() << "\n";
        try
        {
            std::cout << " USB Port ID: " << dev->get_usb_port_id() << "\n";
        }
        catch(...)
        {
        }

        // Show which options are supported by this device
        std::cout << " Supported options:\n";
        for(int j = 0; j < RS_OPTION_COUNT; ++j)
        {
            rs::option opt = (rs::option)j;
            if(dev->supports_option(opt))
            {
                double min, max, step;
                dev->get_option_range(opt, min, max, step);
                std::cout << "    " << opt << " : " << min << " .. " << max << ", " << step << "\n";
            }
        }

        // Show which streams are supported by this device
        for(int j = 0; j < RS_STREAM_COUNT; ++j)
        {
            // Determine number of available streaming modes (zero means stream is unavailable)
            rs::stream strm = (rs::stream)j;
            int mode_count = dev->get_stream_mode_count(strm);
            if(mode_count == 0) continue;

            // Show each available mode for this stream
            std::cout << " Stream " << strm << " - " << mode_count << " modes:\n";
            for(int k = 0; k < mode_count; ++k)
            {
                // Show width, height, format, and framerate, the settings required to enable the stream in this mode
                int width, height, framerate;
                rs::format format;
                dev->get_stream_mode(strm, k, width, height, format, framerate);
                std::cout << "  " << width << "\tx " << height << "\t@ " << framerate << "Hz\t" << format;

                // Enable the stream in this mode so that we can retrieve its intrinsics
                dev->enable_stream(strm, width, height, format, framerate);
                rs::intrinsics intrin = dev->get_stream_intrinsics(strm);

                // Show horizontal and vertical field of view, in degrees
                std::cout << "\t" << std::setprecision(3) << intrin.hfov() << " x " << intrin.vfov() << " degrees\n";
            }

            // Some stream mode combinations are invalid, so disable this stream before moving on to the next one
            dev->disable_stream(strm);
        }
    }

    return EXIT_SUCCESS;
}
catch(const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
