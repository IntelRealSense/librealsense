// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <iostream>
#include <iomanip>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

int main(int argc, char** argv) try
{
    CmdLine cmd("librealsense cpp-enumerate-devices example tool", ' ', RS2_API_VERSION_STR);

    SwitchArg compact_view_arg("s", "short", "Provide short summary of the devices");
    SwitchArg show_options("o", "option", "Show all supported options per subdevice");
    SwitchArg show_modes("m", "modes", "Show all supported stream modes per subdevice");
    cmd.add(compact_view_arg);
    cmd.add(show_options);
    cmd.add(show_modes);

    cmd.parse(argc, argv);

    log_to_console(RS2_LOG_SEVERITY_WARN);

    // Obtain a list of devices currently present on the system
    context ctx;
    auto devices = ctx.query_devices();
    size_t device_count = devices.size();
    if (!device_count)
    {
        printf("No device detected. Is it plugged in?\n");
        return EXIT_SUCCESS;
    }

    if (compact_view_arg.getValue())
    {
        cout << left << setw(30) << "Device Name"
            << setw(20) << "Serial Number"
            << setw(20) << "Firmware Version"
            << endl;

        for (auto i = 0; i < device_count; ++i)
        {
            auto dev = devices[i];

            cout << left << setw(30) << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_NAME)
                << setw(20) << dev.get_camera_info(RS2_CAMERA_INFO_DEVICE_SERIAL_NUMBER)
                << setw(20) << dev.get_camera_info(RS2_CAMERA_INFO_CAMERA_FIRMWARE_VERSION)
                << endl;
        }

        if (show_options.getValue() || show_modes.getValue())
            cout << "\n\nNote:  \"-s\" option is not compatible with the other flags specified,"
                 << " all the additional options are skipped" << endl;

        return EXIT_SUCCESS;
    }

    for (auto i = 0; i < device_count; ++i)
    {
        auto dev = devices[i];

        // Show which options are supported by this device
        cout << " Device info: \n";
        for (auto j = 0; j < RS2_CAMERA_INFO_COUNT; ++j)
        {
            auto param = static_cast<rs2_camera_info>(j);
            if (dev.supports(param))
                cout << "    " << left << setw(30) << rs2_camera_info_to_string(rs2_camera_info(param))
                << ": \t" << dev.get_camera_info(param) << endl;
        }

        cout << endl;

        if (show_options.getValue())
        {
            cout << setw(55) << " Supported options:" << setw(10) << "min" << setw(10)
                << " max" << setw(6) << " step" << setw(10) << " default" << endl;
            for (auto j = 0; j < RS2_OPTION_COUNT; ++j)
            {
                auto opt = static_cast<rs2_option>(j);
                if (dev.supports(opt))
                {
                    auto range = dev.get_option_range(opt);
                    cout << "    " << left << setw(50) << opt << " : "
                        << setw(5) << range.min << "... " << setw(12) << range.max
                        << setw(6) << range.step << setw(10) << range.def << "\n";
                }
            }

            cout << endl;
        }

        if (show_modes.getValue())
        {
            cout << setw(55) << " Supported modes:" << setw(10) << "stream" << setw(10)
                << " resolution" << setw(6) << " fps" << setw(10) << " format" << endl;
            // Show which streams are supported by this device
            for (auto&& profile : dev.get_stream_modes())
            {
                cout << "    " << profile.stream << "\t  " << profile.width << "\tx "
                    << profile.height << "\t@ " << profile.fps << "Hz\t" << profile.format << endl;

                // Show horizontal and vertical field of view, in degrees
                //cout << "\t" << setprecision(3) << intrin.hfov() << " x " << intrin.vfov() << " degrees\n";
            }

            cout << endl;
        }
    }

    cout << endl;

    return EXIT_SUCCESS;
}
catch (const error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
