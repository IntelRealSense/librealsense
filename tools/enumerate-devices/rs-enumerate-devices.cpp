// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <iostream>
#include <iomanip>
#include <map>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

void print(rs2_extrinsics extrinsics)
{
    stringstream ss;
     ss << "Rotation: ";

    for (auto i = 0 ; i < sizeof(extrinsics.rotation)/sizeof(extrinsics.rotation[0]) ; ++i)
        ss << setprecision(15) << extrinsics.rotation[i] << "  ";

    ss << "\nTranslation: ";
    for (auto i = 0 ; i < sizeof(extrinsics.translation)/sizeof(extrinsics.translation[0]) ; ++i)
        ss << setprecision(15) << extrinsics.translation[i] << "  ";

    cout << ss.str() << endl << endl;
}

void print(rs2_motion_device_intrinsic intrinsics)
{
    stringstream ss;
     ss << "Bias Variances: ";

    for (auto i = 0 ; i < sizeof(intrinsics.bias_variances)/sizeof(intrinsics.bias_variances[0]) ; ++i)
        ss << setprecision(15) << intrinsics.bias_variances[i] << "  ";

    ss << "\nNoise Variances: ";
    for (auto i = 0 ; i < sizeof(intrinsics.noise_variances)/sizeof(intrinsics.noise_variances[0]) ; ++i)
        ss << setprecision(15) << intrinsics.noise_variances[i] << "  ";

    ss << "\nData: ";
    for (auto i = 0 ; i < sizeof(intrinsics.data)/sizeof(intrinsics.data[0]) ; ++i)
        ss << setprecision(15) << intrinsics.data[i] << "  ";

    cout << ss.str() << endl << endl;
}

void print(rs2_intrinsics intrinsics)
{
    stringstream ss;
     ss << "Width: "        << intrinsics.width  <<
           "\nHeight: "     << intrinsics.height <<
           "\nPPX: "        << setprecision(15)  << intrinsics.ppx <<
           "\nPPY: "        << setprecision(15)  << intrinsics.ppy <<
           "\nFx: "         << setprecision(15)  << intrinsics.fx  <<
           "\nFy: "         << setprecision(15)  << intrinsics.fy  <<
           "\nDistortion: " << rs2_distortion_to_string(intrinsics.model) <<
           "\nCoeffs: ";

    for (auto i = 0 ; i < sizeof(intrinsics.coeffs)/sizeof(intrinsics.coeffs[0]) ; ++i)
        ss << setprecision(15) << intrinsics.coeffs[i] << "  ";

    cout << ss.str() << endl << endl;
}

int main(int argc, char** argv) try
{
    CmdLine cmd("librealsense cpp-enumerate-devices example tool", ' ', RS2_API_VERSION_STR);

    SwitchArg compact_view_arg("s", "short", "Provide short summary of the devices");
    SwitchArg show_options("o", "option", "Show all supported options per subdevice");
    SwitchArg show_modes("m", "modes", "Show all supported stream modes per subdevice");
    SwitchArg show_calibration_data("c", "calib_data", "Show extrinsics and intrinsics of all subdevices");
    cmd.add(compact_view_arg);
    cmd.add(show_options);
    cmd.add(show_modes);
    cmd.add(show_calibration_data);

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

            cout << left << setw(30) << dev.get_info(RS2_CAMERA_INFO_NAME)
                << setw(20) << dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)
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
                << ": \t" << dev.get_info(param) << endl;
        }

        cout << endl;

        if (show_options.getValue())
        {
            for (auto&& sensor : dev.query_sensors())
            {
                cout << "Options for " << sensor.get_info(RS2_CAMERA_INFO_NAME) << endl;

                cout << setw(55) << " Supported options:" << setw(10) << "min" << setw(10)
                     << " max" << setw(6) << " step" << setw(10) << " default" << endl;
                for (auto j = 0; j < RS2_OPTION_COUNT; ++j)
                {
                    auto opt = static_cast<rs2_option>(j);
                    if (sensor.supports(opt))
                    {
                        auto range = sensor.get_option_range(opt);
                        cout << "    " << left << setw(50) << opt << " : "
                             << setw(5) << range.min << "... " << setw(12) << range.max
                             << setw(6) << range.step << setw(10) << range.def << "\n";
                    }
                }

                cout << endl;
            }
        }

        if (show_modes.getValue())
        {
            for (auto&& sensor : dev.query_sensors())
            {
                cout << "Stream Profiles supported by " << sensor.get_info(RS2_CAMERA_INFO_NAME) << endl;

                cout << setw(55) << " Supported modes:" << setw(10) << "stream" << setw(10)
                     << " resolution" << setw(6) << " fps" << setw(10) << " format" << endl;
                // Show which streams are supported by this device
                for (auto&& profile : sensor.get_stream_profiles())
                {
                    if (auto video = profile.as<video_stream_profile>())
                    {
                        cout << "    " << profile.stream_name() << "\t  " << video.width() << "x"
                            << video.height() << "\t@ " << profile.fps() << "Hz\t" << profile.format() << endl;
                    }
                    else
                    {
                        cout << "    " << profile.stream_name() << "\t@ " << profile.fps() << "Hz\t" << profile.format() << endl;
                    }
                }

                cout << endl;
            }
        }

        // Print Intrinsics
        if (show_calibration_data.getValue())
        {
            for (auto&& sensor : dev.query_sensors())
            {
                cout << "Intrinsics provided by " << sensor.get_info(RS2_CAMERA_INFO_NAME) << endl;

                // Intrinsics
                for (auto&& profile : sensor.get_stream_profiles())
                {
                    if (auto video = profile.as<video_stream_profile>())
                    {
                        cout << "Intrinsics of " << profile.stream_name() << "\t  " << video.width() << "x"
                            << video.height() << "\t@ " << profile.fps() << "Hz\t" << profile.format() << endl;

                        print(video.get_intrinsics());
                    }
                }
            }
        }
    }

    // Print Extrinsics
    if (show_calibration_data.getValue())
    {
        throw std::runtime_error("TODO: Rewrite Extrinsics info!");
    }

    cout << endl;

    return EXIT_SUCCESS;
}
catch (const error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
