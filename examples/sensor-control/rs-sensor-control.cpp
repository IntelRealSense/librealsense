// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <iomanip>
#include <map>
#include <utility>
#include <vector>
#include <librealsense2/rs.hpp>
#include "../example.hpp"

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

class frames_viewer
{
    std::atomic_bool _stop;
    texture renderer;
    rs2::colorizer colorize;
    std::string _window_title;
    frames_holder _frames;
    std::unique_ptr<std::thread> _thread;
    std::mutex _mutex;
    std::condition_variable _cv;
public:
    frames_viewer(const std::string& window_title):
        _window_title(window_title), 
        _thread(new std::thread(&frames_viewer::run, this)),
        _stop(false)
    {
    }
    ~frames_viewer()
    {
        stop();
        if (_thread && _thread->joinable())
            _thread->join();
        
    }
    void operator()(rs2::frame f)
    {
        _frames.put(f);
    }
    void stop()
    {
        _stop = true;
    }
    void wait()
    {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [&]() { return _stop.load(); });
    }
private:
    void run()
    {
        window app(640, 480, _window_title.c_str());

        while (app && !_stop) // Application still alive?
        {
            auto last_frames = _frames.get();
            auto total_number_of_streams = last_frames.size();
            int cols = int(std::ceil(std::sqrt(total_number_of_streams)));
            int rows = int(std::ceil(total_number_of_streams / static_cast<float>(cols)));

            float view_width = (app.width() / cols);
            float view_height = (app.height() / rows);
            int stream_no = 0;
            for (auto&& f : last_frames)
            {
                rect frame_location{ view_width * (stream_no % cols), view_height * (stream_no / cols), view_width, view_height };
                try
                {
                    renderer.upload(colorize(f));
                    if (rs2::video_frame vid_frame = f.as<rs2::video_frame>())
                    {
                        rect adjuested = frame_location.adjust_ratio({ static_cast<float>(vid_frame.get_width())
                            , static_cast<float>(vid_frame.get_height()) });
                        renderer.show(adjuested);
                    }
                }
                catch (const std::exception& e)
                {
                    draw_text(frame_location.x + (std::max(0.f, (frame_location.w / 2) - std::string(e.what()).length() * 3)),
                        frame_location.y + int(frame_location.h / 2), e.what());
                }
                stream_no++;
            }
        }
        std::lock_guard<std::mutex> lock(_mutex);
        _stop = true;
        _cv.notify_one();
    }
};

bool prompt_yes_no(const std::string& prompt_msg)
{
    char ans;
    do
    {
        std::cout << prompt_msg << "[y/n]: ";
        std::cin >> ans;
        std::cout << std::endl;
    } while (!std::cin.fail() && ans != 'y' && ans != 'n');
    return ans == 'y';
}
uint32_t get_user_selection(const std::string& prompt_message)
{
    std::cout << "\n" << prompt_message;
    uint32_t input;
    std::cin >> input;
    std::cout << std::endl;
    return input;
}
void print_seperator()
{
    std::cout << "\n======================================================\n" << std::endl;
}
class how_to
{
public:
    static rs2::device get_a_realsense_device()
    {
        // First, create a rs2::context.
        // The context represents the current platform with respect to connected devices
        rs2::context ctx;

        // Using the context we can get all connected devices in a device list
        rs2::device_list devices = ctx.query_devices();

        if (devices.size() == 0)
        {
            throw std::runtime_error("No device connected, please connect a RealSense device");
        }

        std::cout << "Found the following devices:\n" << std::endl;

        // device_list is a "lazy" container of devices which allows
        //The device list provides 2 ways of iterating it
        //The first way is using an iterator (in this case hidden in the Range-based for loop)
        int index = 0;
        for (rs2::device device : devices)
        {
            std::cout << "  " << index++ << " : " << get_device_name(device) << std::endl;
        }

        uint32_t selected_device_index = get_user_selection("Select a device by index: ");
        
        // The second way is using the subscript ("[]") operator:
        if (selected_device_index >= devices.size())
        {
            throw std::out_of_range("Selected device index is out of range");
        }

        return  devices[selected_device_index];
    }
    static void print_device_information(const rs2::device& dev)
    {
        // Each device provides some information on itself
        // The different types of available information are represented using the "RS2_CAMERA_INFO_*" enum

        std::cout << "Device information: " << std::endl;
        //The following code shows how to enumarate all of the RS2_CAMERA_INFO
        //Note that all enum types in the SDK start with the value of zero and end at the "*_COUNT" value
        for (int i = 0; i < static_cast<int>(RS2_CAMERA_INFO_COUNT); i++)
        {
            rs2_camera_info info_type = static_cast<rs2_camera_info>(i);
            //SDK enum types can be streamed to get a string that represents them
            std::cout << "\t" << std::left << std::setw(20) << info_type << " : ";

            //A device might not support all types of RS2_CAMERA_INFO.
            //To prevent throwing exceptions from the "get_info" method we first check if the device supports this type of info
            if (dev.supports(info_type))
                std::cout << dev.get_info(info_type) << std::endl;
            else
                std::cout << "N/A" << std::endl;
        }
    }
    static std::string get_device_name(const rs2::device& dev)
    {
        // Each device provides some information on itself, such as name:
        std::string name = "Unknown Device";
        if (dev.supports(RS2_CAMERA_INFO_NAME))
            name = dev.get_info(RS2_CAMERA_INFO_NAME);

        std::string sn = "########";
        if (dev.supports(RS2_CAMERA_INFO_SERIAL_NUMBER))
            sn = std::string("#") + dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);

        return name + " " + sn;
    }
    static std::string get_sensor_name(const rs2::sensor& sensor)
    {
        // Sensors support additional information, such as a human readable name
        if (sensor.supports(RS2_CAMERA_INFO_NAME))
            return sensor.get_info(RS2_CAMERA_INFO_NAME);
        else
            return "Unknown Sensor";
    }
    static rs2::sensor get_a_sensor_from_a_device(const rs2::device& dev)
    {
        // Get all sensors of a device:
        std::vector<rs2::sensor> sensors = dev.query_sensors();

        std::cout << "Device consists of " << sensors.size() << " sensors:\n" << std::endl;
        int index = 0;
        for (rs2::sensor sensor : sensors)
        {
            std::cout << "  " << index++ << " : " << get_sensor_name(sensor) << std::endl;
        }

        uint32_t selected_sensor_index = get_user_selection("Select a sensor by index: ");

        // The second way is using the subscript ("[]") operator:
        if (selected_sensor_index >= sensors.size())
        {
            throw std::out_of_range("Selected sensor index is out of range");
        }

        return  sensors[selected_sensor_index];
    }
    static rs2_option get_sensor_option(const rs2::sensor& sensor)
    {
        std::cout << "Sensor supports the following options:\n" << std::endl;

        for (int i = 0; i < static_cast<int>(RS2_OPTION_COUNT); i++)
        {
            rs2_option option_type = static_cast<rs2_option>(i);
            std::cout << "  " << i << ": " << option_type;

            // To control an option, use the following api:
            if (sensor.supports(option_type))
            {
                std::cout << " : " << std::endl;

                // Get a human readable description of the option
                const char* description = sensor.get_option_description(option_type);
                std::cout << "      Description : " << description << std::endl;
                // Get the current value of the option
                float current_value = sensor.get_option(option_type);
                std::cout << "      Current Value : " << current_value << std::endl;
            }
            else
            {
                std::cout << " is not supported" << std::endl;
            }
        }
        uint32_t selected_sensor_option = get_user_selection("Select an option by index: ");
        if (selected_sensor_option >= static_cast<int>(RS2_OPTION_COUNT))
        {
            throw std::out_of_range("Selected option is out of range");
        }
        return static_cast<rs2_option>(selected_sensor_option);
    }
    static float get_depth_units(const rs2::sensor& sensor)
    {
        //A Depth stream contains an image that is composed of pixels with depth information.
        //The value of each pixel is the distance from the camera, in some distance units.
        //To get the distance in units of meters, each pixel's value should be multiplied by the sensor's depth scale
        //Here is the way to grab this scale value for a "depth" sensor:
        if (rs2::depth_sensor dpt_sensor = sensor.as<rs2::depth_sensor>())
        {
            float scale = dpt_sensor.get_depth_scale();
            std::cout << "Scale factor for depth sensor is: " << scale << std::endl;
            return scale;
        }
        else
            throw std::runtime_error("Given sensor is not a depth sensor");
    }
    static rs2_intrinsics get_field_of_view(const rs2::stream_profile& stream)
    {
        if (auto video_stream = stream.as<rs2::video_stream_profile>())
        {
            rs2_intrinsics intrinsics = video_stream.get_intrinsics();
            auto principal_point = std::make_pair(intrinsics.ppx, intrinsics.ppy);
            auto focal_length = std::make_pair(intrinsics.fx, intrinsics.fy);
            rs2_distortion model = intrinsics.model;
            return intrinsics;
        }
        else
            throw std::runtime_error("Given stream profile is not a video stream profile");
    }
    static void change_sensor_option(const rs2::sensor& sensor, rs2_option option_type)
    {
        // To control an option, use the following api:
        if (!sensor.supports(option_type))
        {
            std::cerr << "This option is not supported by this sensor" << std::endl;
            return;
        }

        // Each option provides its rs2::option_range to provide information on how it can be changed
        // To get the supported range of an option we do the following:

        std::cout << "Supported range for option " << option_type << ":" << std::endl;

        rs2::option_range range = sensor.get_option_range(option_type);
        float default_value = range.def;
        float maximum_supported_value = range.max;
        float minimum_supported_value = range.min;
        float difference_to_next_value = range.step;
        std::cout << "  Min Value     : " << minimum_supported_value << std::endl;
        std::cout << "  Max Value     : " << maximum_supported_value << std::endl;
        std::cout << "  Default Value : " << default_value << std::endl;
        std::cout << "  Step          : " << difference_to_next_value << std::endl;

        bool change_option = false;
        change_option = prompt_yes_no("Change option's value?");

        if (change_option)
        {
            std::cout << "Enter the new value for this option: ";
                float requested_value;
            std::cin >> requested_value;
            std::cout << std::endl;

            // To set an option to a different value, we can call set_option with a new value
            try
            {
                sensor.set_option(option_type, requested_value);
            }
            catch (const rs2::error& e)
            {
                // Some options can only be set while the camera is streaming, 
                // and generally the hardware might fail so it is good practice to catch exceptions from set_option
                std::cerr << "Failed to set option " << option_type << ". (" << e.what() << ")" << std::endl;
            }
        }
    }
    static void start_streaming_a_profile(const rs2::sensor& sensor)
    {
        // Using the sensor we can get all of its streaming profiles
        std::vector<rs2::stream_profile> stream_profiles = sensor.get_stream_profiles();

        // Usually a sensor provides one or more streams which are identifiable by their stream_type and stream_index
        // Each of these streams can have several profiles (e.g FHD/HHD/VGA/QVGA resolution, or 90/60/30 fps, etc..)
        //The following 
        std::map<std::pair<rs2_stream, int>, int> unique_streams;
        for (auto&& sp : stream_profiles)
        {
            unique_streams[std::make_pair(sp.stream_type(), sp.stream_index())]++;
        }
        std::cout << "Sensor consists of " << unique_streams.size() << " streams: " << std::endl;
        for (size_t i = 0; i < unique_streams.size(); i++)
        {
            auto it = unique_streams.begin();
            std::advance(it, i);
            std::cout << "  - " << it->first.first << " #" << it->first.second << std::endl;
        }

        std::cout << "Sensor provides the following stream profiles:" << std::endl;
        // We can iterate over the available profiles of a sensor
        int profile_num = 0;
        for (rs2::stream_profile stream_profile : stream_profiles)
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
            std::cout << std::setw(3) << profile_num << ": " << stream_data_type << " #" << stream_index;

            // As noted, a stream is an abstraction.
            // In order to get additional data for the specific type of a
            //  stream, a mechanism of "Is" and "As" is provided:
            if (stream_profile.is<rs2::video_stream_profile>()) //"Is" will test if the type tested is of the type given
            {
                // "As" will convert the instance to the given type
                rs2::video_stream_profile video_stream_profile = stream_profile.as<rs2::video_stream_profile>();

                // After using the "as" method we can use the new data type
                //  for additinal operations:
                std::cout << " (Video Stream: " << video_stream_profile.format() << " " <<
                    video_stream_profile.width() << "x" << video_stream_profile.height() << "@ " << video_stream_profile.fps() << "Hz)";
            }
            std::cout << std::endl;
            profile_num++;
        }

        uint32_t selected_profile_index = get_user_selection("Please select a streaming profile to play: ");
        if (selected_profile_index >= stream_profiles.size())
        {
            std::cerr << "Requested profile index is out of range" << std::endl;
            return;
        }
        rs2::stream_profile selected_profile = stream_profiles[selected_profile_index];
        sensor.open(selected_profile);
        std::ostringstream oss;
        oss << "Displaying profile: " << selected_profile.stream_name();
        frames_viewer display(oss.str());
        sensor.start([&](rs2::frame f) { display(f); });
        std::cout << "Streaming profile: " << selected_profile.stream_name() << ". Close display window to continue..." << std::endl;
        display.wait();
        sensor.stop();
        sensor.close();
    }
};



int main(int argc, char * argv[]) try
{
    //In this example we will go throw a flow of selecting a device, then selecting one of its sensors,
    // and finally run some operations on that sensor

    //We will use the "how_to" class to make the code clear, expressive and encapsulate common actions inside a function

    bool choose_a_device = true;
    while (choose_a_device)
    {
        print_seperator();
        //First thing, let's choose a device:
        rs2::device dev = how_to::get_a_realsense_device();

        //Print the device's information
        how_to::print_device_information(dev);

        // A rs2::device is a container of rs2::sensors that share some correlation between them.
        // For example:
        //    * A device where all sensors are on a single board
        //    * A Robot with mounted sensors that share calibration information

        bool choose_a_sensor = true;
        while (choose_a_sensor)
        {
            print_seperator();
            //Next, choose one of the device's sensors
            rs2::sensor sensor = how_to::get_a_sensor_from_a_device(dev);
            // A Sensor is an object that is capable of streaming one or more
            //  types of data.
            // For example:
            //    * A stereo sensor with Left and Right Infrared streams that
            //        creates a stream of depth images
            //    * A motion sensor with an Accelerometer and Gyroscope that
            //        provides a stream of motion information

            bool choose_an_option = true;
            while (choose_an_option)
            {
                print_seperator();
                // The rs2::sensor allows you to control its properties such as Exposure, Brightness etc.
                rs2_option sensor_option = how_to::get_sensor_option(sensor);
                how_to::change_sensor_option(sensor, sensor_option);
                choose_an_option = prompt_yes_no("Choose another option?");
            }

            bool choose_a_stream_profile = true;
            while (choose_a_stream_profile)
            {
                print_seperator();
                // The rs2::sensor allows you to control its properties such as Exposure, Brightness etc.
                how_to::start_streaming_a_profile(sensor);
                choose_a_stream_profile = prompt_yes_no("Choose another profile?");
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
