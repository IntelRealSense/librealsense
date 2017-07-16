// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.


#include <iostream>
#include <iomanip>
#include <thread>
#include <librealsense/rs2.hpp>
#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

int main(int argc, const char** argv) try
{
    //Add switches to this example to allow user to indicate play mode, and file path
    CmdLine cmd("librealsense rs-recording example", ' ', RS2_API_VERSION_STR);

    ValueArg<std::string> file_path("f", "file_path", "File path for recording output", false, "record_example.bag", "string");
    cmd.add( file_path );

    vector<string> allowed_modes;
    allowed_modes.push_back("record");
    allowed_modes.push_back("playback");
    allowed_modes.push_back("live");
    ValuesConstraint<string> modes( allowed_modes );
    ValueArg<string> nameArg("m","mode","Indicate the mode to run this example", true, "record",&modes);
    cmd.add( nameArg );

    cmd.parse(argc, argv);

    context ctx;
    auto devices = ctx.query_devices();
    if (devices.size() == 0)
    {
        std::cerr << "No device connected" << std::endl;
        return EXIT_FAILURE;
    }

    device device;
    if(nameArg.getValue()== "record")
    {
        //Create a recorder from the device
        device = recorder(file_path.getValue(), devices[0]);
        std::cout << "Recording to file: " << file_path.getValue() << std::endl;
    }
    else if(nameArg.getValue() == "playback")
    {
        //Create a playback device from file
        device = playback(file_path.getValue());
        std::cout << "Playing from file: " << file_path.getValue() << std::endl;
    }
    else if(nameArg.getValue() == "live")
    {
        device = devices[0];
        std::cout << "Streaming from actual device" << std::endl;
    }
    else
    {
        throw std::runtime_error(modes.description() + " is not a supported mode");
    }

    std::cout << "Device: " << device.get_info(RS2_CAMERA_INFO_NAME) << std::endl;

    //Declare profiles we want to play
    const std::vector<rs2::stream_profile> profiles_to_play_if_available{
        rs2::stream_profile{ RS2_STREAM_DEPTH, 640, 480, 30, RS2_FORMAT_Z16 },
        rs2::stream_profile{ RS2_STREAM_INFRARED, 640, 480, 30, RS2_FORMAT_Y8 },
        rs2::stream_profile{ RS2_STREAM_FISHEYE, 640, 480, 30, RS2_FORMAT_RAW8 },
        rs2::stream_profile{ RS2_STREAM_COLOR, 640, 480, 30, RS2_FORMAT_RGBA8 }
    };
    
    std::vector<sensor> m_playing_sensors; //will hold the sensors that are playing
    int sensor_id = 0;
    //Go over the sensors and open start streaming
    for (auto&& sensor : device.query_sensors())
    {
        sensor_id++;
        if (sensor.supports(RS2_CAMERA_INFO_NAME))
        {
            std::cout << "Sensor #" << sensor_id << ": " << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
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

    std::this_thread::sleep_for(std::chrono::seconds(10));
    
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