// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
// Minimalistic command-line collect & analyze bandwidth/performance tool for Realsense Cameras.
// The data is gathered and serialized in csv-compatible format for offline analysis.
// Extract and store frame headers info for video streams; for IMU&Tracking streams also store the actual data
// The utility is configured with command-line keys and requires user-provided config file to run
// See rs-data-collect.h for config examples

#include <librealsense2/rs.hpp>
#include "rs-data-collect.h"
#include "tclap/CmdLine.h"
#include <thread>
#include <iostream>

using namespace std;
using namespace TCLAP;

enum application_stop : uint8_t {
    stop_on_frame_num,
    stop_on_timeout,
    stop_on_user_frame_num,
    stop_on_any
};

using namespace rs_data_collect;

int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-data-collect example tool", ' ');
    ValueArg<int>    timeout("t", "Timeout", "Max amount of time to receive frames (in seconds)", false, 10, "");
    ValueArg<int>    max_frames("m", "MaxFrames_Number", "Maximum number of frames-per-stream to receive", false, 100, "");
    ValueArg<string> filename("f", "FullFilePath", "the file where the data will be saved to", false, "", "");
    ValueArg<string> config_file("c", "ConfigurationFile", "Specify file path with the requested configuration", false, "", "");

    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(filename);
    cmd.add(config_file);
    cmd.parse(argc, argv);

    std::cout << "Running rs-data-collect: ";
    for (auto i=1; i < argc; ++i)
        std::cout << argv[i] << " ";
    std::cout << std::endl;

    auto output_file       = filename.isSet() ? filename.getValue() : DEF_OUTPUT_FILE_NAME;
    auto max_frames_number = max_frames.isSet() ? max_frames.getValue() :
                                timeout.isSet() ? std::numeric_limits<uint64_t>::max() : DEF_FRAMES_NUMBER;

    auto stop_cond = static_cast<application_stop>((max_frames.isSet() << 1) | timeout.isSet());

    bool succeed = false;
    rs2::context ctx;
    rs2::device_list list;
    std::vector<rs2::sensor> active_sensors;

    while (!succeed)
    {
        std::vector<stream_request> requests_to_go, user_requests;
        list = ctx.query_devices();

        if (0== list.size())
        {
            std::cout << "Connect Realsense Camera to proceed" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }

        auto dev = std::make_shared<rs2::device>(list.front());

        if (config_file.isSet())
        {
            parse_requests(user_requests, config_file.getValue());
        }

        if (!config_file.isSet() || !user_requests.size())
            throw std::runtime_error(stringify()
                    << "The tool requires a profile configuration file to proceed"
                    << "\nRun rs-data-collect --help for details");

        requests_to_go = user_requests;
        std::vector<rs2::stream_profile> matches, total_matches;

        // Configure and starts streaming
        for (auto&& sensor : dev->query_sensors())
        {
            for (auto& profile : sensor.get_stream_profiles())
            {
                // All requests have been resolved
                if (!requests_to_go.size())
                    break;

                // Find profile matches
                auto fulfilled_request = std::find_if(requests_to_go.begin(), requests_to_go.end(), [&matches, profile](const stream_request& req)
                {
                    bool res = false;
                    if ((profile.stream_type()  == req._stream_type) &&
                        (profile.format()       == req._stream_format) &&
                        (profile.stream_index() == req._stream_idx) &&
                        (profile.fps()          == req._fps))
                    {
                        if (auto vp = profile.as<rs2::video_stream_profile>())
                        {
                            if ((vp.width() != req._width) || (vp.height() != req._height))
                                return false;
                        }
                        res = true;
                        matches.push_back(profile);
                    }

                    return res;
                });

                // Remove the request once resolved
                if (fulfilled_request != requests_to_go.end())
                    requests_to_go.erase(fulfilled_request);
            }

            // Aggregate resolved requests
            if (matches.size())
            {
                std::copy(matches.begin(), matches.end(), std::back_inserter(total_matches));
                sensor.open(matches);
                active_sensors.emplace_back(sensor);
                matches.clear();
            }

            if (total_matches.size() == user_requests.size())
                succeed = true;
        }

        std::cout << "\nDevice selected: \n\t" << dev->get_info(RS2_CAMERA_INFO_NAME)
                << (dev->supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR) ?
                    std::string(( stringify() << ". USB Type: " << dev->get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))) : "")
                << "\n\tS.N: " << (dev->supports(RS2_CAMERA_INFO_SERIAL_NUMBER) ? dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "")
                << "\n\tFW Ver: " << dev->get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)
                << "\nUser streams requested: " << user_requests.size()
                << ", actual matches: " << total_matches.size() << std::endl;

        if (!succeed)
        {
            std::cout << "\nThe following request(s) are not supported by the device: " << std::endl;
            for(auto& elem : requests_to_go)
                std::cout << elem;

            if (requests_to_go.size() == user_requests.size())
                throw std::runtime_error("\nNo valid stream requests were found, aborting..");
        }

        data_collection buffer;
        auto start_time = chrono::high_resolution_clock::now();

        // Start streaming
        for (auto&& sensor : active_sensors)
            sensor.start([&](rs2::frame f)
        {
            auto arrival_time = std::chrono::duration<double, std::milli>(chrono::high_resolution_clock::now() - start_time);
            auto stream_uid = std::make_pair(f.get_profile().stream_type(), f.get_profile().stream_index());

            if (auto motion = f.as<rs2::motion_frame>())
            {
                auto axes = motion.get_motion_data();

                if (buffer[stream_uid].size() < max_frames_number)
                {
                    buffer[stream_uid].push_back(std::make_shared<threedof_data>(f.get_frame_number(),
                        f.get_timestamp(),
                        arrival_time.count(),
                        f.get_frame_timestamp_domain(),
                        f.get_profile().stream_type(),
                        f.get_profile().stream_index(),
                        axes.x, axes.y, axes.z));
                }
            }
            else
            {
                if (auto pf = f.as<rs2::pose_frame>())
                {
                    if (buffer[stream_uid].size() < max_frames_number)
                    {
                        auto pose = pf.get_pose_data();
                        buffer[stream_uid].push_back(std::make_shared<sixdof_data>(pf.get_frame_number(),
                            pf.get_timestamp(),
                            arrival_time.count(),
                            f.get_frame_timestamp_domain(),
                            f.get_profile().stream_type(),
                            f.get_profile().stream_index(),
                            pose.translation.x, pose.translation.y, pose.translation.z,
                            pose.rotation.x,pose.rotation.y,pose.rotation.z,pose.rotation.w));
                    }
                }
                else
                {
                    if (buffer[stream_uid].size() < max_frames_number)
                    {
                        buffer[stream_uid].push_back(std::make_shared<frame_data>( f.get_frame_number(),
                                f.get_timestamp(),
                                arrival_time.count(),
                                f.get_frame_timestamp_domain(),
                                f.get_profile().stream_type(),
                                f.get_profile().stream_index()));
                    }
                }
            }
        });

        std::cout << "\nData collection started.... " << std::endl;
        bool timed_out = false;

        const auto collection_accomplished = [&]()
        {
            //If none of the (timeout/num of frames) switches is set, terminate after max_frames_number.
            //If both switches are set, then terminate of whatever comes first
            //Otherwise, ready according to the switch specified
            timed_out = false;
            if (timeout.isSet())
            {
                auto timeout_sec = std::chrono::seconds(timeout.getValue());
                timed_out = (chrono::high_resolution_clock::now() - start_time >= timeout_sec);

                // When timeout only option is specified, disregard frame number
                if (stop_on_timeout==stop_cond)
                    return timed_out;
            }

            bool collected_enough_frames = true;
            for (auto&& match : total_matches)
            {
                auto key = std::make_pair(match.stream_type(),match.stream_index());
                if (!buffer.size() || (buffer.find(key) !=buffer.end() &&
                    (buffer[key].size() && buffer[key].size() < max_frames_number)))
                {
                    collected_enough_frames = false;
                    break;
                }
            }

             return timed_out || collected_enough_frames;
        };

        while (!collection_accomplished())
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Collecting data for "
                    << chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count()
                    << " sec" << std::endl;
        }

        // Stop & flush all active sensors
        for (auto&& sensor : active_sensors)
        {
            sensor.stop();
            sensor.close();
        }

        std::vector<uint64_t> frames_per_stream;
        for (const auto& kv : buffer)
            frames_per_stream.emplace_back(kv.second.size());

        std::sort(frames_per_stream.begin(),frames_per_stream.end());
        std::cout   << "\nData collection accomplished with ["
                    << frames_per_stream.front() << "-" << frames_per_stream.back()
                    << "] frames recorded per stream\nSerializing captured results to"
                    << output_file << std::endl;

        save_data_to_file(buffer, total_matches, output_file);

        succeed = true;
    }
    std::cout << "Task completed" << std::endl;

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}
catch (const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
