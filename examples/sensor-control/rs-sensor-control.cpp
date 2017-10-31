// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <iomanip>
#include <map>
#include <utility>
#include <vector>
#include <librealsense2/rs.hpp>
#include <algorithm>
#include "../example.hpp"
#include "api_how_to.h"
#include "helper.h"

using namespace helper;

std::vector<std::pair<std::function<void(rs2::device, rs2::sensor)>, std::string>> create_sensor_actions();

int main(int argc, char * argv[]) try
{
    //In this example we will go throw a flow of selecting a device, then selecting one of its sensors,
    // and finally run some operations on that sensor

    //We will use the "how_to" class to make the code clear, expressive and encapsulate common actions inside a function
    
    auto sensor_actions = create_sensor_actions();

    bool choose_a_device = true;
    while (choose_a_device)
    {
        print_seperator();
        //First thing, let's choose a device:
        rs2::device dev = how_to::get_a_realsense_device();

        //Print the device's information
        how_to::print_device_information(dev);

        bool choose_a_sensor = true;
        while (choose_a_sensor)
        {
            print_seperator();
            //Next, choose one of the device's sensors
            rs2::sensor sensor = how_to::get_a_sensor_from_a_device(dev);

            bool control_sensor = true;
            while (control_sensor)
            {
                print_seperator();
                std::cout << "What would you like to do with the sensor?\n" << std::endl;
                int i = 0;
                for (auto&& action : sensor_actions)
                {
                    std::cout << i++ << " : " << action.second << std::endl;
                }
                uint32_t selected_action_index = helper::get_user_selection("Select an action: ");
                if (selected_action_index >= sensor_actions.size())
                {
                    throw std::out_of_range("Selected action index is out of range");
                }

                auto selected_action = sensor_actions[selected_action_index].first;

                selected_action(dev, sensor);

                control_sensor = prompt_yes_no("Choose another action for this sensor?");
            }

            choose_a_sensor = prompt_yes_no("Choose another sensor?");
        }

        choose_a_device = prompt_yes_no("Choose another device?");
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

std::vector<std::pair<std::function<void(rs2::device, rs2::sensor)>, std::string>> create_sensor_actions()
{
    std::vector<std::pair<std::function<void(rs2::device, rs2::sensor)>, std::string>> actions;
    
    auto options_control = std::make_pair([](rs2::device ignored, rs2::sensor sensor) {
        bool repeat = true;
        while (repeat)
        {
            print_seperator();
            // The rs2::sensor allows you to control its properties such as Exposure, Brightness etc.
            rs2_option sensor_option = how_to::get_sensor_option(sensor);
            how_to::change_sensor_option(sensor, sensor_option);
            repeat = prompt_yes_no("Choose another option?");
        }
    }, "Control sensor's options");

    actions.push_back(options_control);

    auto stream_control = std::make_pair([](rs2::device ignored, rs2::sensor sensor) {
        bool repeat = true;
        while (repeat)
        {
            print_seperator();
            // The rs2::sensor allows you to control its streams
            // We will first choose a single stream profile from the available profiles of the sensor
            rs2::stream_profile selected_profile = how_to::choose_a_streaming_profile(sensor);

            // Next, we will display the stream in a window
            how_to::start_streaming_a_profile(sensor, selected_profile);
            repeat = prompt_yes_no("Choose another profile?");
        }
    }, "Control sensor's streams");

    actions.push_back(stream_control);

    auto display_intrinsics = std::make_pair([](rs2::device ignored, rs2::sensor sensor) {
        bool repeat = true;
        while (repeat)
        {
            print_seperator();
            rs2::stream_profile selected_profile = how_to::choose_a_streaming_profile(sensor);
            // The rs2::sensor allows you to control its properties such as Exposure, Brightness etc.
            how_to::get_field_of_view(selected_profile);
            repeat = prompt_yes_no("Choose another profile?");
        }
    }, "Display intrinsics");

    actions.push_back(display_intrinsics);

    auto display_extrinsics = std::make_pair([](rs2::device dev, rs2::sensor ignored) {
        bool repeat = true;
        while (repeat)
        {
            print_seperator();
            std::cout << "Please choose a sensor and then a stream that will be used as the origin of extrinsic transformation:\n" << std::endl;
            rs2::sensor from_sensor = how_to::get_a_sensor_from_a_device(dev);
            rs2::stream_profile from = how_to::choose_a_streaming_profile(from_sensor);

            std::cout << "Please choose a sensor and then a stream that will be used as the target of extrinsic transformation::\n" << std::endl;
            rs2::sensor to_sensor = how_to::get_a_sensor_from_a_device(dev);
            rs2::stream_profile to = how_to::choose_a_streaming_profile(to_sensor);
            // The rs2::sensor allows you to control its properties such as Exposure, Brightness etc.
            how_to::get_extrinsics(from, to);
            repeat = prompt_yes_no("Choose other profiles?");
        }
    }, "Display extrinsics");

    actions.push_back(display_extrinsics);
    return actions;
}
