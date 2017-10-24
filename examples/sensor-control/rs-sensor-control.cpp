// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <sstream>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cstring>
#include <thread>
#include <array>
#include <librealsense2/rs.hpp>


//A simple C++ functor that handles frames (and does nothing with them)
//An example for using this class can be found below
class my_frame_handler_class
{
public:
    void operator()(rs2::frame f)
    {
        //Handle frame here...
    }
};

//A simple free function that handles frames (and does nothing with them)
//An example for using this function can be found below
void frame_handler_that_does_nothing(rs2::frame f)
{
    //Handle frame here...
}
bool is_desired_stream(const rs2::video_stream_profile& video_stream_profile)
{
    return 
            //Has VGA resolution
            (video_stream_profile.width() == 640 && video_stream_profile.height() == 480)
        &&
            //And is one of the following streams
            (
                (video_stream_profile.stream_type() == RS2_STREAM_DEPTH &&
                 video_stream_profile.format() == RS2_FORMAT_Z16)
            ||
                (video_stream_profile.stream_type() == RS2_STREAM_INFRARED &&
                 video_stream_profile.stream_index() == 1 &&
                 video_stream_profile.format() == RS2_FORMAT_Y8)
            ||
                (video_stream_profile.stream_type() == RS2_STREAM_COLOR &&
                 video_stream_profile.format() == RS2_FORMAT_RGB8)
            );
}

int main(int argc, char * argv[]) try
{
    // First, create a rs2::context.
    // The context represents the current platform with respect to
    //  connected devices
    rs2::context ctx;

    // Using the context we can get all connected devices in a device list
    rs2::device_list devices = ctx.query_devices();

    // device_list is a "lazy" container of devices which allows
    // iteration over the devices
    for (rs2::device dev : devices)
    {
        // Each device provides some information on itself, such as name:
        if (dev.supports(RS2_CAMERA_INFO_NAME))
            std::cout << "Device name: " << dev.get_info(RS2_CAMERA_INFO_NAME) << std::endl;

        // Serial number
        if (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
            std::cout << "Device serial number: " << dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << std::endl;

        // Firmware version, and more...
        if (dev.supports(RS2_CAMERA_INFO_FIRMWARE_VERSION))
            std::cout << "Device firmware version: " << dev.get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << std::endl;

        // A rs2::device is a container of rs2::sensors that share some
        //  correlation between them.
        // For example:
        //    * A device where all sensors are on a single board
        //    * A Robot with mounted sensors that share calibration information

        std::vector<rs2::sensor> sensors = dev.query_sensors();
        for (rs2::sensor sensor : sensors)
        {
            // A Sensor is an object that is capable of streaming one or more
            //  types of data.
            // For example:
            //    * A stereo sensor with Left and Right Infrared streams that
            //        creates a stream of depth images
            //    * A motion sensor with an Accelerometer and Gyroscope that
            //        provides a stream of motion information

            // Just like the device, a sensor provides additional information
            //  on itself
            if (sensor.supports(RS2_CAMERA_INFO_NAME))
                std::cout << "\tSensor name: " << sensor.get_info(RS2_CAMERA_INFO_NAME) << std::endl;

            // In addition to information of a sensor, sensors support options
            // control such as Exposure, Brightness etc.
            // The following loop shows how to iterate over all available options
            // Starting from 0 until RS2_OPTION_COUNT (exclusive)
            for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
            {
                rs2_option option_type = static_cast<rs2_option>(i);
                // To control an option, use the following api:
                if (sensor.supports(option_type))
                {
                    // Get a human readable description of the option
                    const char* description = sensor.get_option_description(option_type);

                    // Get the current value of the option
                    float current_value = sensor.get_option(option_type);

                    // Get the supported range of the option
                    rs2::option_range range = sensor.get_option_range(option_type);
                    float default_value = range.def;
                    float maximum_supported_value = range.max;
                    float minimum_supported_value = range.min;
                    float difference_to_next_value = range.step;

                    // To set an option to a different value, we can call set_
                    //  option with a new value
                    // In this example we set each option to its default value
                    try
                    {
                        sensor.set_option(option_type, default_value);
                    }
                    catch (const rs2::error& e)
                    {
                        // Some options can only be set while the camera is
                        //  streaming (and we haven't started streaming yet)
                    }
                }
            }
            std::vector<rs2::stream_profile> streams_to_open;
            // We can iterate over the available profiles of a sensor
            for (rs2::stream_profile stream_profile : sensor.get_stream_profiles())
            {
                // A Stream is an abstraction for a sequence of data items of a
                //  single data type, which are ordered according to their time
                //  of creation or arrival.
                // The stream's data types are represented using the rs2_stream
                //  enumeration
                rs2_stream stream_data_type = stream_profile.stream_type();

                // The rs2_stream provides only types of data which are
                //  supported by the RealSense SDK
                // For example:
                //    * rs2_stream::RS2_STREAM_DEPTH describes a stream of depth images
                //    * rs2_stream::rs2_stream::RS2_STREAM_COLOR describes a stream of color images
                //    * rs2_stream::rs2_stream::RS2_STREAM_INFRARED describes a stream of infrared images

                // As mentioned, a sensor can have multiple streams.
                // In order to distinguish between streams with the same
                //  stream type we can use the following methods:

                // 1) Each stream type can have multiple occurances.
                //    All streams, of the same type, provided from a single
                //     device have distinc indices:
                int stream_index = stream_profile.stream_index();

                // 2) Each stream has a user friendly name.
                //    The stream's name is not promised to be unique,
                //     rather a human readable description of the stream
                std::string stream_name = stream_profile.stream_name();

                // 3) Each stream in the system, which derives from the same
                //     rs2::context, has a unique identifier
                //    This identifier is unique across all streams, regardless of the stream type.
                int unique_stream_id = stream_profile.unique_id(); // The unique identifier can be used for comparing two streams

                std::cout << "\t\tStream #" << unique_stream_id
                     << " is " << stream_data_type
                     << " #" << stream_index
                     << ", Named: \"" << stream_name << "\"" << std::endl;

                // As noted, a stream is an abstraction.
                // In order to get additional data for the specific type of a
                //  stream, a mechanism of "Is" and "As" is provided:
                if (stream_profile.is<rs2::video_stream_profile>()) //"Is" will test if the type tested is of the type given
                {
                    // "As" will convert the instance to the given type
                    rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();

                    // After using the "as" method we can use the new data type
                    //  for additinal operations:
                    std::cout << "\t\t\tThis stream is a video stream representing a stream of images with a resolution of "
                        << video_stream_profile.width() << "x" << video_stream_profile.height() <<
                        ", a frame rate of " << video_stream_profile.fps() << " frames per second"
                        ", and a pixel format of: " << video_stream_profile.format() << std::endl;

                    if (is_desired_stream(video_stream_profile))
                    {
                        streams_to_open.push_back(video_stream_profile);
                    }
                }
            }

            // The sensor controls turning the streaming on and off
            // To start streaming, two calls must be made with the following order:
            //  1) open(stream_profiles_to_open)
            //  2) start(function_to_handle_frames)

            // In the following code we start the sensor with the first profile it provides
            if (sensor.get_stream_profiles().size() > 0)
            {
                // Open can be called with a single profile, or with a collection of profiles
                // Calling open() tries to get exclusive access to the sensor.
                // Opening a sensor may have side effects such as actually
                //  running, consume power, produce data, etc.
                sensor.open(streams_to_open);

                // In order to begin getting data from the sensor, we need to register a callback to handle frames (data)
                // To register a callback, the sensor's start() method should be invoked.
                // The start() method takes any type of callable object that takes a frame as its parameter
                // NOTE:
                //  * Since a sensor can stream multiple streams, and start
                //     takes a single handler, multiple types of frames can
                //     arrive to the handler.
                //  * Different streams' frames arrive on different threads.
                //    This behavior requires the provided frame handler to the
                //     start method to be re-entrant
                // A lambda (that prints the frame number)
                sensor.start([](rs2::frame f)
                {
                    std::cout << "Frame received #" << f.get_frame_number()
                        << " with stream type: " << f.get_profile().stream_type() << std::endl;
                });

                try
                {
                    // A function pointer
                    sensor.start(frame_handler_that_does_nothing);

                    // A functor
                    my_frame_handler_class my_frame_handler_instance;
                    sensor.start(my_frame_handler_instance);
                }
                catch (const rs2::error& e)
                {
                    // We wrapped the calls to start() with try{} catch{} block for 2 reasons here:
                    //  1) calling start() multiple times throws an exception (but we wanted to show that it compiles with different types)
                    //  2) Start can throw an exception generally.

                    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
                }

                // After calling start, frames will begin to arrive asynchronously to the callback handler.
                // We now block the main thread from continuing, to allow frames to arrive during this time
                std::this_thread::sleep_for(std::chrono::seconds(5));

                // To stop streaming, we simply need to call the sensor's stop method
                // After returning from the call to stop(), no frames will arrive from this sensor
                sensor.stop();

                // To complete the stop operation, and release access of the device, we need to call close() per sensor
                sensor.close();
            }
        }
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
