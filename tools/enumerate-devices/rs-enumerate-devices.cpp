// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <iostream>
#include <iomanip>
#include <map>
#include <set>
#include <cstring>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

void print(const rs2_extrinsics& extrinsics)
{
    stringstream ss;
     ss << "Rotation Matrix:\n";

    for (auto i = 0 ; i < 3 ; ++i)
    {
        for (auto j = 0 ; j < 3 ; ++j)
        {
            ss << left << setw(15) << setprecision(5) << extrinsics.rotation[j*3 +i];
        }
        ss << endl;
    }

    ss << "\nTranslation Vector: ";
    for (auto i = 0 ; i < sizeof(extrinsics.translation)/sizeof(extrinsics.translation[0]) ; ++i)
        ss << setprecision(15) << extrinsics.translation[i] << "  ";

    cout << ss.str() << endl << endl;
}

void print(const rs2_motion_device_intrinsic& intrinsics)
{
    stringstream ss;
     ss << "Bias Variances: ";

    for (auto i = 0 ; i < sizeof(intrinsics.bias_variances)/sizeof(intrinsics.bias_variances[0]) ; ++i)
        ss << setprecision(15) << intrinsics.bias_variances[i] << "  ";

    ss << "\nNoise Variances: ";
    for (auto i = 0 ; i < sizeof(intrinsics.noise_variances)/sizeof(intrinsics.noise_variances[0]) ; ++i)
        ss << setprecision(15) << intrinsics.noise_variances[i] << "  ";

    ss << "\nData: " << std::endl;
    for (auto i = 0 ; i < sizeof(intrinsics.data)/sizeof(intrinsics.data[0]) ; ++i)
    {
        for (auto j = 0 ; j < sizeof(intrinsics.data[0])/sizeof(intrinsics.data[0][0]) ; ++j)
            ss << std::setw(13) << setprecision(10) << intrinsics.data[i][j] << "  ";
        ss << "\n";
    }


    cout << ss.str() << endl << endl;
}

void print(const rs2_intrinsics& intrinsics)
{
    stringstream ss;
     ss << left << setw(14) << "Width: "      << "\t" << intrinsics.width  << endl <<
           left << setw(14) << "Height: "     << "\t" << intrinsics.height << endl <<
           left << setw(14) << "PPX: "        << "\t" << setprecision(15)  << intrinsics.ppx << endl <<
           left << setw(14) << "PPY: "        << "\t" << setprecision(15)  << intrinsics.ppy << endl <<
           left << setw(14) << "Fx: "         << "\t" << setprecision(15)  << intrinsics.fx  << endl <<
           left << setw(14) << "Fy: "         << "\t" << setprecision(15)  << intrinsics.fy  << endl <<
           left << setw(14) << "Distortion: " << "\t" << rs2_distortion_to_string(intrinsics.model) << endl <<
           left << setw(14) << "Coeffs: ";

    for (auto i = 0 ; i < sizeof(intrinsics.coeffs)/sizeof(intrinsics.coeffs[0]) ; ++i)
        ss << "\t" << setprecision(15) << intrinsics.coeffs[i] << "  ";

    cout << ss.str() << endl << endl;
}

bool safe_get_intrinsics(const video_stream_profile& profile, rs2_intrinsics& intrinsics)
{
    bool ret = false;
    try
    {
        intrinsics = profile.get_intrinsics();
        ret = true;
    }
    catch(...)
    {}

    return ret;
}

bool safe_get_motion_intrinsics(const motion_stream_profile& profile, rs2_motion_device_intrinsic& intrinsics)
{
    bool ret = false;
    try
    {
        intrinsics = profile.get_motion_intrinsics();
        ret = true;
    }
    catch(...)
    {}
    return ret;
}

struct stream_and_resolution{
    rs2_stream stream;
    int stream_index;
    int width;
    int height;
    string stream_name;

    bool operator <(const stream_and_resolution& obj) const
    {
        return (std::make_tuple(stream, stream_index, width, height) < std::make_tuple(obj.stream, obj.stream_index, obj.width, obj.height));
    }
};

struct stream_and_index{
    rs2_stream stream;
    int stream_index;

    bool operator <(const stream_and_index& obj) const
    {
        return (std::make_tuple(stream, stream_index) < std::make_tuple(obj.stream, obj.stream_index));
    }
};

bool operator ==(const rs2_intrinsics& lhs,
                 const rs2_intrinsics& rhs)
{
    return lhs.width == rhs.width &&
           lhs.height == rhs.height &&
           lhs.ppx == rhs.ppx &&
           lhs.ppy == rhs.ppy &&
           lhs.fx == rhs.fx &&
           lhs.fy == rhs.fy &&
           lhs.model == rhs.model &&
           !std::memcmp(lhs.coeffs, rhs.coeffs, sizeof(rhs.coeffs));
}

bool operator ==(const rs2_motion_device_intrinsic& lhs,
                 const rs2_motion_device_intrinsic& rhs)
{
    return !std::memcmp(&lhs, &rhs, sizeof(rs2_motion_device_intrinsic));
}

string get_str_formats(const set<rs2_format>& formats)
{
    stringstream ss;
    for (auto format = formats.begin(); format != formats.end(); ++format)
    {
        ss << *format << ((format != formats.end()) && (next(format) == formats.end())?"":"/");
    }
    return ss.str();
}

int main(int argc, char** argv) try
{
    CmdLine cmd("librealsense rs-enumerate-devices example tool", ' ', RS2_API_VERSION_STR);

    SwitchArg compact_view_arg("s", "short", "Provide short summary of the devices");
    SwitchArg show_options("o", "option", "Show all supported options per subdevice");
    SwitchArg show_modes("m", "modes", "Show all supported stream modes per subdevice");
    SwitchArg show_calibration_data("c", "calib_data", "Show extrinsic and intrinsic of all subdevices");
    cmd.add(compact_view_arg);
    cmd.add(show_options);
    cmd.add(show_modes);
    cmd.add(show_calibration_data);

    cmd.parse(argc, argv);

    log_to_console(RS2_LOG_SEVERITY_ERROR);

    // Obtain a list of devices currently present on the system
    context ctx;
    auto devices = ctx.query_devices();
    size_t device_count = devices.size();
    if (!device_count)
    {
        cout <<"No device detected. Is it plugged in?\n";
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
                << setw(20) << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
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

        // Print Intrinsics
        if (show_calibration_data.getValue())
        {
            std::map<stream_and_index, stream_profile> streams;
            std::map<stream_and_resolution, std::vector<std::pair<std::set<rs2_format>, rs2_intrinsics>>> intrinsics_map;
            std::map<stream_and_resolution, std::vector<std::pair<std::set<rs2_format>, rs2_motion_device_intrinsic>>> motion_intrinsics_map;
            for (auto&& sensor : dev.query_sensors())
            {
                // Intrinsics
                for (auto&& profile : sensor.get_stream_profiles())
                {
                    if (auto video = profile.as<video_stream_profile>())
                    {
                        if (streams.find(stream_and_index{profile.stream_type(), profile.stream_index()}) == streams.end())
                        {
                            streams[stream_and_index{profile.stream_type(), profile.stream_index()}] = profile;
                        }

                        rs2_intrinsics intrinsics{};
                        stream_and_resolution stream_res{profile.stream_type(), profile.stream_index(), video.width(), video.height(), profile.stream_name()};
                        if (safe_get_intrinsics(video, intrinsics))
                        {
                            auto it = std::find_if((intrinsics_map[stream_res]).begin(), (intrinsics_map[stream_res]).end(), [&](const std::pair<std::set<rs2_format>, rs2_intrinsics>& kvp) {
                                return intrinsics == kvp.second;
                            });
                            if (it == (intrinsics_map[stream_res]).end())
                            {
                                (intrinsics_map[stream_res]).push_back({ {profile.format()}, intrinsics });
                            }
                            else
                            {
                                it->first.insert(profile.format()); // If the intrinsics are equals, add the profile format to format set
                            }
                        }
                    }
                    else
                    {
                        if (motion_stream_profile motion = profile.as<motion_stream_profile>())
                        {
                            if (streams.find(stream_and_index{profile.stream_type(), profile.stream_index()}) == streams.end())
                            {
                                streams[stream_and_index{profile.stream_type(), profile.stream_index()}] = profile;
                            }

                            rs2_motion_device_intrinsic motion_intrinsics{};
                            stream_and_resolution stream_res{profile.stream_type(), profile.stream_index(), motion.stream_type(), motion.stream_index(), profile.stream_name()};
                            if (safe_get_motion_intrinsics(motion, motion_intrinsics))
                            {
                                auto it = std::find_if((motion_intrinsics_map[stream_res]).begin(), (motion_intrinsics_map[stream_res]).end(),
                                    [&](const std::pair<std::set<rs2_format>, rs2_motion_device_intrinsic>& kvp)
                                {
                                    return motion_intrinsics == kvp.second;
                                });
                                if (it == (motion_intrinsics_map[stream_res]).end())
                                {
                                    (motion_intrinsics_map[stream_res]).push_back({ {profile.format()}, motion_intrinsics });
                                }
                                else
                                {
                                    it->first.insert(profile.format()); // If the intrinsics are equals, add the profile format to format set
                                }
                            }
                        }
                    }
                }
            }

            cout << "Provided Intrinsic:" << endl;
            for (auto& kvp : intrinsics_map)
            {
                auto stream_res = kvp.first;
                for (auto& intrinsics : kvp.second)
                {
                    auto formats = get_str_formats(intrinsics.first);
                    cout << "Intrinsic of \"" << stream_res.stream_name << "\"\t  " << stream_res.width << "x"
                        << stream_res.height << "\t  " << formats << endl;
                    if (intrinsics.second == rs2_intrinsics{})
                    {
                        cout << "Intrinsic NOT available!\n\n";
                    }
                    else
                    {
                        print(intrinsics.second);
                    }
                }
            }

            cout << "Provided Motion Intrinsic:" << endl;
            for (auto& kvp : motion_intrinsics_map)
            {
                auto stream_res = kvp.first;
                for (auto& intrinsics : kvp.second)
                {
                    auto formats = get_str_formats(intrinsics.first);
                    cout << "Motion Intrinsic of \"" << stream_res.stream_name << "\"\t  " << formats << endl;
                    if (intrinsics.second == rs2_motion_device_intrinsic{})
                    {
                        cout << "Intrinsic NOT available!\n\n";
                    }
                    else
                    {
                        print(intrinsics.second);
                    }
                }
            }

            // Print Extrinsics
            cout << "\nProvided Extrinsic:" << endl;
            rs2_extrinsics extrinsics{};
            for (auto kvp1 = streams.begin(); kvp1 != streams.end(); ++kvp1)
            {
                for (auto kvp2 = streams.begin(); kvp2 != streams.end(); ++kvp2)
                {
                    cout << "Extrinsic from \"" << kvp1->second.stream_name() << "\"\t  " <<
                            "To" << "\t  \"" << kvp2->second.stream_name() << "\" :\n";
                    try
                    {
                        extrinsics = kvp1->second.get_extrinsics_to(kvp2->second);
                        print(extrinsics);
                    }
                    catch (...)
                    {
                        cout << "N/A\n";
                    }
                }
            }
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
