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

    std::cout << "Recording to file: " << file_path.getValue() << std::endl;

    context ctx;
    auto devices = ctx.query_devices();
    if (devices.size() == 0)
    {
        std::cerr << "No device connected" << std::endl;
        return EXIT_FAILURE;
    }
    //Create a recorder from the device
    //recorder device(file_path.getValue(), devices[0]);
    playback device(file_path.getValue());
    //From this point on we use the record_device and not the (live) device

   // std::cout << "Device: " << device.get_info(RS2_CAMERA_INFO_NAME) << std::endl;

    //Declare profiles we want to play
    const std::vector<rs2::stream_profile> profiles_to_play_if_available{
        rs2::stream_profile{ RS2_STREAM_DEPTH, 640, 480, 30, RS2_FORMAT_Z16 },
        rs2::stream_profile{ RS2_STREAM_INFRARED, 640, 480, 30, RS2_FORMAT_Y8 },
        rs2::stream_profile{ RS2_STREAM_FISHEYE, 640, 480, 30, RS2_FORMAT_RAW8 },
        rs2::stream_profile{ RS2_STREAM_COLOR, 640, 480, 30, RS2_FORMAT_RGBA8 }
    };
    
    std::vector<sensor> m_playing_sensors; //will hold the sensors that are playing

    //Go over the sensors and open start streaming
    for (auto&& sensor : device.query_sensors())
    {
        //if (sensor.supports(RS2_CAMERA_INFO_NAME))
        //{
        //    std::cout << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
        //}
            
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
            std::cout << rs2_stream_to_string(f.get_stream_type()) << " frame #" << f.get_frame_number() <<  std::endl;
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
    getchar();
    //std::this_thread::sleep_for(std::chrono::seconds(5));
    
    for(auto sensor : m_playing_sensors)
    {
        sensor.stop();
        sensor.close();
    }
    
    return 0;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}