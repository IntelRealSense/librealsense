// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <iostream>
#include <iomanip>
#include <core/streaming.h> //TODO: remove
#include <sensor.h>
#include <record_device.h>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

int main(int argc, const char** argv) try
{
    CmdLine cmd("librealsense cpp-record example", ' ', RS2_API_VERSION_STR);
    ValueArg<std::string> file_path("f", "file_path", "File path for recording output", false, "record_example.bag", "string");
    cmd.add( file_path );
    cmd.parse(argc, argv);

    const std::vector<rs2::stream_profile> profiles_to_play_if_available{
        rs2::stream_profile{ RS2_STREAM_DEPTH, 640, 480, 30, RS2_FORMAT_Z16 },
        rs2::stream_profile{ RS2_STREAM_INFRARED, 640, 480, 30, RS2_FORMAT_Y8 },
        rs2::stream_profile{ RS2_STREAM_FISHEYE, 640, 480, 30, RS2_FORMAT_RAW8 }
    };

    std::vector<sensor> m_playing_sensors;
    context ctx;
    //for(auto&& device : ctx.query_devices())
    {
        auto device = ctx.query_devices()[1];
        recorder record_device(file_path.getValue(), device);
        for (auto&& sensor : record_device.query_sensors())
        {
            if (sensor.supports(RS2_CAMERA_INFO_NAME))
            {
                std::cout << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
            }
            
            std::vector<rs2::stream_profile> profiles_to_play_for_this_sensor;
            for (auto profile : sensor.get_stream_modes())
            {
                if (std::find(std::begin(profiles_to_play_if_available), 
                    std::end(profiles_to_play_if_available), profile) != std::end(profiles_to_play_if_available))
                {
                    profiles_to_play_for_this_sensor.push_back(profile);
                }

            }
            if (profiles_to_play_for_this_sensor.empty())
            {
                //This sensor does not support any of the requested profiles
                continue;
            }
            sensor.open(profiles_to_play_for_this_sensor);
            sensor.start([](rs2::frame f) {
                std::cout << "Got frame #" << f.get_frame_number() << rs2_stream_to_string(f.get_stream_type()) << std::endl;
            });
            m_playing_sensors.push_back(sensor);
            try
            {
                roi_sensor x(sensor);
            }
            catch (const std::exception& e)
            {
                std::cout << e.what() << std::endl;
            }
        }
    }
    getchar();
    
    for(auto sensor : m_playing_sensors)
    {
        sensor.stop();
        sensor.close();
    }
    std::cout << "Recording to file: " << file_path.getValue() << std::endl;
    return 0;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}