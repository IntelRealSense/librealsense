// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include "tclap/CmdLine.h"
#include <condition_variable>
#include <set>
#include <cctype>
#include <thread>
#include <array>
#include <map>
#include <atomic>
#include <regex>
#include <algorithm>    // std::sort


using namespace std;
using namespace TCLAP;

int MAX_FRAMES_NUMBER = 10; //number of frames to capture from each stream
const unsigned int NUM_OF_STREAMS = static_cast<int>(RS2_STREAM_COUNT);

// The code is "borrowed" from win-helpers.cpp core library file
std::vector<std::string> tokenize(std::string string, char separator)
{
    std::vector<std::string> tokens;
    std::string::size_type i1 = 0;
    while(true)
    {
        auto i2 = string.find(separator, i1);
        if(i2 == std::string::npos)
        {
            tokens.push_back(string.substr(i1));
            return tokens;
        }
        tokens.push_back(string.substr(i1, i2-i1));
        i1 = i2+1;
    }
}

// This code also appears in rendering.h
struct stringify
{
    std::ostringstream ss;
    template<class T> stringify & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

struct frame_data
{
    unsigned long long      frame_number;
    double                  ts;
    long long               arrival_time;
    rs2_timestamp_domain    domain;
    rs2_stream              stream_type;
};

struct stream_request
{
    rs2_stream  _stream_type;
    rs2_format  _stream_format;
    int         _width;
    int         _height;
    int         _fps;
    int         _stream_idx;
};

inline std::ostream&  operator<<(std::ostream& os, const stream_request& req)
{
    return os << "Type: " << req._stream_type << ",Idx: " << req._stream_idx << ", "
        << req._stream_format << ", [" << req._width << "x" << req._height << "], " << req._fps << "fps" << std::endl;
}

//Defines the order at which the params (comma-separated) should appear in configuration file
//#rs2_stream, width, height, fps, rs2_format, stream_index (optional, put zero or leave empty if you're not sure)
//# Examples (note that all lines starting with non-alpha character (#@!...) will be skipped:
//  DEPTH,640,480,30,Z16,0
//  INFRARED,640,480,30,Y8,1
//  COLOR,640,480,30,RGB8,0
//  ACCEL,1,1,125,MOTION_XYZ32F
//  GYRO,1,1,200,MOTION_XYZ32F
enum config_params { STREAM_TYPE = 0, RES_WIDTH, RES_HEIGHT, FPS, FORMAT, STREAM_INDEX };

int parse_number(char const *s, int base = 0)
{
    char c;
    stringstream ss(s);
    int i;
    ss >> i;

    if (ss.fail() || ss.get(c))
    {
        throw runtime_error(string(string("Invalid numeric input - ") + s + string("\n")));
    }
    return i;
}

std::string to_lower(const std::string& s)
{
    auto copy = s;
    std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
    return copy;
}

rs2_format parse_format(const string str)
{
    for (int i = RS2_FORMAT_ANY; i < RS2_FORMAT_COUNT; i++)
    {
        if (to_lower(rs2_format_to_string((rs2_format)i)) == str)
        {
            return (rs2_format)i;
        }
    }
    throw runtime_error((string("Invalid format - ") + str + string("\n")).c_str());
}

rs2_stream parse_stream_type(const string str)
{
    for (int i = RS2_STREAM_ANY; i < RS2_STREAM_COUNT; i++)
    {
        if (to_lower(rs2_stream_to_string((rs2_stream)i)) == str)
        {
            return static_cast<rs2_stream>(i);
        }
    }
    throw runtime_error((string("Invalid stream type - ") + str + string("\n")).c_str());
}

int parse_fps(const string str)
{
    return parse_number(str.c_str());
}

bool parse_configuration(const std::string& line, rs2_stream& type, int& width, int& height, rs2_format& format, int& fps, int& index)
{
    bool res = false;
    try
    {
        auto tokens = tokenize(line,',');
        if (tokens.size() < STREAM_INDEX)
            return res;

        // Convert string to uppercase
        type    =   parse_stream_type(to_lower(tokens[STREAM_TYPE]));
        width   =   parse_number(tokens[RES_WIDTH].c_str());
        height  =   parse_number(tokens[RES_HEIGHT].c_str());
        fps     =   parse_fps(tokens[FPS]);
        format  =   parse_format(to_lower(tokens[FORMAT]));
        // Backward compatibility
        if (tokens.size() > STREAM_INDEX)
            index = parse_number(tokens[STREAM_INDEX].c_str());
        res = true;
        std::cout << "Request added for " << line << std::endl;
    }
    catch (...)
    {
        std::cout << "Invalid syntax in configuration line " << line << std::endl;
    }

    return res;
}

void parse_requests(std::vector<stream_request>& reqs, const std::string& config_filename)
{
    ifstream file(config_filename);

    if (!file.is_open())
        throw runtime_error(stringify() << "Given .csv configure file " << config_filename <<  " was not Found!");

    // Parse data collecion requests
    string line;
    while (getline(file, line))
    {
        // Parsing configuration requests
        stringstream ss(line);
        vector<string> row;
        while (ss.good())
        {
            string substr;
            getline(ss, substr, ',');
            row.push_back(substr);
        }

        rs2_stream stream_type;
        rs2_format format;
        int width{}, height{}, fps{}, index{};

        // correctness check
        // Line starting with non-alpha characters will be treated as comments
        std::regex starts_with("^[a-zA-Z]");
        if (std::regex_search(line, starts_with))
        {
            if (parse_configuration(line, stream_type, width, height, format, fps,index))
                reqs.push_back({ stream_type, format, width, height, fps,  index });
        }
    }

    // negate multiple requests
    if (reqs.size())
    {
        std::sort(reqs.begin(), reqs.end(), [](const stream_request& l, const stream_request& r) { return l._stream_type < r._stream_type; });
        for (auto i = 0; i < reqs.size() - 1; i++)
        {
            if ((reqs[i]._stream_type == reqs[i+1]._stream_type) && ((reqs[i]._stream_idx == reqs[i+1]._stream_idx)))
                throw runtime_error(stringify() << "Invalid configuration file - multiple requests for the same sensor:\n"
                    << reqs[i] << "\n" << reqs[+i]);
        }
    }
    else
        throw std::runtime_error("Invalid configuration file - zero requests accepted");
}

void save_data_to_file(std::array<list<frame_data>, NUM_OF_STREAMS> buffer, const string& filename)
{
    // Save to file
    ofstream csv;
    csv.open(filename);
    csv << "Stream Type,F#,Timestamp,Arrival Time\n";

    for (int stream_index = 0; stream_index < NUM_OF_STREAMS; stream_index++)
    {
        auto buffer_size = buffer[stream_index].size();
        for (auto i = 0; i < buffer_size; i++)
        {
            ostringstream line;
            auto data = buffer[stream_index].front();
            line << rs2_stream_to_string(data.stream_type) << "," << data.frame_number << "," << std::fixed << std::setprecision(3) << data.ts << "," << data.arrival_time << "\n";
            buffer[stream_index].pop_front();
            csv << line.str();
        }
    }

    csv.close();
}

int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-data-collect example tool", ' ');
    ValueArg<int>    timeout("t", "Timeout", "Max amount of time to receive frames (in seconds)", false, 10, "");
    ValueArg<int>    max_frames("m", "MaxFrames_Number", "Maximum number of frames data to receive", false, 100, "");
    ValueArg<string> filename("f", "FullFilePath", "the file which the data will be saved to", false, "", "");
    ValueArg<string> config_file("c", "ConfigurationFile", "Specify file path with the requested configuration", false, "", "");

    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(filename);
    cmd.add(config_file);
    cmd.parse(argc, argv);

    std::string output_file = filename.isSet() ? filename.getValue() : "frames_data.csv";

    auto max_frames_number = MAX_FRAMES_NUMBER;

    if (max_frames.isSet())
        max_frames_number = max_frames.getValue();

    bool succeed = false;

    rs2::context ctx;
    rs2::device_list list;

    while (!succeed)
    {
        std::vector<stream_request> requests, orig_requests;
        
        list = ctx.query_devices();

        if (0== list.size())
            continue;

        auto dev = std::make_shared<rs2::device>(list.front());

        if (config_file.isSet())
        {
            parse_requests(requests, config_file.getValue());
        }

//        rs2::pipeline_profile profile = pipe.start(config);
//        auto dev = profile.get_device();

        std::atomic_bool need_to_reset(false);
        for (auto&& sensor : dev->query_sensors())
        {
            sensor.set_notifications_callback([&](const rs2::notification& n)
            {
                if (n.get_category() == RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT)
                {
                    need_to_reset = true;
                }
            });
        }

		orig_requests = requests;
		std::vector<rs2::stream_profile> matches, total_matches;
        // Configure and starts streaming
        for (auto&& sensor : dev->query_sensors())
        {
            auto profiles = sensor.get_stream_profiles();
            std::cout << "Sensor with " << profiles.size() << " profiles" << std::endl;

			std::cout << sensor.get_info(rs2_camera_info::RS2_CAMERA_INFO_NAME) << std::endl;
			int i = 0;
            for (auto& profile : profiles)
            {
				i++;

				// All requests have been resolved
				if (!requests.size())
					break;

                /*std::cout << "Basic params: " << profile.stream_type() << ", " << profile.stream_name() << ", "
                    << profile.format() << ", " << profile.fps();*/
                if (auto vp = profile.as<rs2::video_stream_profile>())
                {
                    std::cout << ", UVC params: " << vp.width() << ", " << vp.height();
                }

                if (auto mp = profile.as<rs2::motion_stream_profile>())
                {
                    std::cout << ", IMU profile: " << std::endl;
                }
                std::cout << std::endl;

                // Map user requests to available streams
				
                //for_each(requests.begin(), requests.end(), [&matches, profile](const stream_request& req)
                auto fulfilled_request = std::find_if(requests.begin(), requests.end(), [&matches, profile, i,sensor](const stream_request& req)
                {
                    bool res = false;
                    if (auto vp = profile.as<rs2::video_stream_profile>())
                    {
                        rs2_stream type = vp.stream_type();
                        rs2_format format = vp.format();
                        int index   = vp.stream_index();
                        int fps     = vp.fps();
                        int width = vp.width();
                        int height = vp.height();

						std::cout << " Check profile " << i << ": "
							<< type << ", " << format << ", index: " << index << ", "
							<< fps << ", " << width << "x" << height
							<< " against:" << req._stream_type << ", " << req._stream_format << ", index: " << req._stream_idx << ", "
							<< req._fps << ", " << req._width << "x" << req._height;
						if ((vp.stream_type() == req._stream_type) &&
							(vp.format() == req._stream_format) &&
							(vp.stream_index() == req._stream_idx) &&
							(vp.fps() == req._fps) &&
							(vp.width() == req._width) &&
							(vp.height() == req._height))
						{
							matches.push_back(profile);
							
							res = true;
							std::cout << " Bingo!!!!!!!";
						}
						std::cout << std::endl;

                    }
                    else
                    {
                        if (auto mp = profile.as<rs2::motion_stream_profile>())
                        {
                            if ((mp.stream_type() == req._stream_type) &&
                                (mp.format() == req._stream_format) &&
                                (mp.stream_index() == req._stream_idx) &&
                                (mp.fps() == req._fps))
                            {
                                matches.push_back(profile);
                                res = true;
                            }
                        }
                    }
                    return res;
                });

				if (fulfilled_request != requests.end())
					requests.erase(fulfilled_request);
            }

			// Aggregate resolved requests
			if (matches.size())
			{
				std::copy(matches.begin(), matches.end(), std::back_inserter(total_matches));
				sensor.open(matches);
				matches.clear();
			}

			if (total_matches.size() == orig_requests.size())
				succeed = true;
        }

		std::cout << "final matches are " << total_matches.size() << std::endl;

        // Start streaming
		for (auto&& sensor : dev->query_sensors())
			sensor.start([&](rs2::frame f) 
		{
			if (auto motion = f.as<rs2::motion_frame>())
			{
			    auto axes = motion.get_motion_data();
			    std::cout
			    << "Frame type, " << f.get_profile().stream_type()
			    << " , num, " << f.get_frame_number()
			    << " ,ts, " << std::fixed << f.get_timestamp()
			    << " ,x, " <<  axes.x
			    << " ,y, " <<  axes.y
			    << " ,z, " <<  axes.z
			    << std::endl;
			}

			if (auto video = f.as<rs2::video_frame>())
			{
				std::cout
					<< "Frame type, " << f.get_profile().stream_type()
					<< " format, " << f.get_profile().format()
					<< " , num, " << f.get_frame_number()
					<< " ,ts, " << std::fixed << f.get_timestamp()
					<< std::endl;
			}
			/*frame_data data{f.get_frame_number(),
			                f.get_timestamp(),
			                arrival_time.count(),
			                f.get_frame_timestamp_domain(),
			                f.get_profile().stream_type()};
			if (buffer[(int)data.stream_type].size() < max_frames_number)
			{
			    buffer[(int)data.stream_type].push_back(data);
			}*/
		});
        

        std::array<std::list<frame_data>, NUM_OF_STREAMS> buffer;
        auto start_time = chrono::high_resolution_clock::now();
        const auto ready = [&]()
        {
            //If not switch is set, we're ready after max_frames_number frames.
            //If both switches are set, we're ready when the first of the two conditions is true
            //Otherwise, ready according to given switch
            bool timed_out = false;
            if (timeout.isSet())
            {
                auto timeout_sec = std::chrono::seconds(timeout.getValue());
                timed_out = (chrono::high_resolution_clock::now() - start_time >= timeout_sec);
            }

			bool collected_enough_frames = false;// Evgeni true;
            // TODO Evgeni re-enable
//            for (auto&& profile : pipe.get_active_profile().get_streams())
//            {
//                if (buffer[(int)profile.stream_type()].size() < max_frames_number)
//                {
//                    collected_enough_frames = false;
//                }
//            }

            if (timeout.isSet() && !max_frames.isSet())
                return timed_out;

            return timed_out || collected_enough_frames;
        };

//        while (!ready())
//        {
//            auto frameset = pipe.wait_for_frames();
//            auto arrival_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time);

//            for (auto it = frameset.begin(); it != frameset.end(); ++it)
//            {
//                auto f = (*it);
//                if (auto motion = f.as<rs2::motion_frame>())
//                {
//                    auto axes = motion.get_motion_data();
//                    std::cout
//                    << "Frame type, " << f.get_profile().stream_type()
//                    << " , num, " << f.get_frame_number()
//                    << " ,ts, " << std::fixed << f.get_timestamp()
//                    << " ,x, " <<  axes.x
//                    << " ,y, " <<  axes.y
//                    << " ,z, " <<  axes.z
//                    << std::endl;
//                }
//                frame_data data{f.get_frame_number(),
//                                f.get_timestamp(),
//                                arrival_time.count(),
//                                f.get_frame_timestamp_domain(),
//                                f.get_profile().stream_type()};
//                if (buffer[(int)data.stream_type].size() < max_frames_number)
//                {
//                    buffer[(int)data.stream_type].push_back(data);
//                }
//            }

//            if (need_to_reset)
//            {
//                dev.hardware_reset();
//                need_to_reset = false;
//                break;
//            }
//        }

		while (!ready())
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

        save_data_to_file(buffer, output_file);
		for (auto&& sensor : dev->query_sensors())
			sensor.stop();
        succeed = true;
    }
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
