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

// Default frames to capture if user does not supply alternatives
const unsigned int MAX_FRAMES_NUMBER = 100;

// The code is "borrowed" from win-helpers.cpp core library file
std::vector<std::string> tokenize(std::string line, char separator)
{
    std::vector<std::string> tokens;

    // Remove trailing characters
    line.erase(line.find_last_not_of("\t\n\v\r ") + 1);

    stringstream ss(line);
    while (ss.good())
    {
        string substr;
        getline(ss, substr, separator);
        tokens.push_back(substr);
    }

    return tokens;
}

struct stringify
{
    std::ostringstream ss;
    template<class T> stringify & operator << (const T & val) { ss << val; return *this; }
    operator std::string() const { return ss.str(); }
};

struct frame_data
{
    frame_data(unsigned long long frame_number,
               double frame_ts,
               double host_ts,
               rs2_timestamp_domain domain,
               rs2_stream stream_type,
               int stream_index):
    _frame_number(frame_number),
    _ts(frame_ts),
    _arrival_time(host_ts),
    _domain(domain),
    _stream_type(stream_type),
    _stream_idx(stream_index)
    {};

    unsigned long long      _frame_number;
    double                  _ts;
    double                  _arrival_time;
    rs2_timestamp_domain    _domain;
    rs2_stream              _stream_type;
    int                     _stream_idx;
};

inline std::ostream & operator << (std::ostream & out, frame_data data)
{
    return out  << "\n" << rs2_stream_to_string(data._stream_type) << ","
                << data._stream_idx << ","
                << data._frame_number << ","
                << std::fixed << std::setprecision(3)
                << data._ts << "," << data._arrival_time;
}

struct threedof_data : frame_data
{
    threedof_data(unsigned long long frame_number,
               double frame_ts,
               double host_ts,
               rs2_timestamp_domain domain,
               rs2_stream stream_type,
               int stream_index,
               double x, double y, double z):
        frame_data(frame_number, frame_ts, host_ts, domain, stream_type, stream_index),
        _x_axis(x), _y_axis(y), _z_axis(z)
        {};

    double  _x_axis;
    double  _y_axis;
    double  _z_axis;
};

inline std::ostream & operator << (std::ostream & os, threedof_data data)
{
    return os  << static_cast<frame_data>(data) << ","
                << data._x_axis << "," << data._y_axis << "," << data._z_axis;
}

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

bool parse_configuration(const std::string& line, const std::vector<std::string>& tokens,
    rs2_stream& type, int& width, int& height, rs2_format& format, int& fps, int& index)
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

    // Line starting with non-alpha characters will be treated as comments
    const std::regex starts_with("^[a-zA-Z]");
    string line;
    rs2_stream stream_type;
    rs2_format format;
    int width{}, height{}, fps{}, index{};

    while (getline(file, line))
    {
        // Parsing configuration requests
        auto tokens =tokenize(line,',');

        if (std::regex_search(line, starts_with))
        {
            if (parse_configuration(line, tokens, stream_type, width, height, format, fps,index))
                reqs.push_back({ stream_type, format, width, height, fps,  index });
        }
    }

    // Sanity test agains multiple conflicting requests for single sensor
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

std::string get_profile_description(const rs2::stream_profile& profile)
{
    std::stringstream ss;

    ss  << profile.stream_name() << ","
        << profile.stream_type() << ","
        << profile.format() << ","
        << profile.fps();

    if (auto vp = profile.as<rs2::video_stream_profile>())
        ss << "," << vp.width() << "," << vp.height();

    ss << std::endl;
    return ss.str().c_str();
}

void save_data_to_file(const std::map<std::pair<rs2_stream, int>, std::vector<frame_data>>& buffer,
                        const std::vector<rs2::stream_profile>& active_stream_profiles,
                        const string& out_filename)
{
    // Save/override to file
    ofstream csv(out_filename);
    //csv.open(out_filename);

    csv << "Configuration:\nStream Type,Stream Name,Format,FPS,Width,Height\n";
    for (const auto& elem : active_stream_profiles)
        csv << get_profile_description(elem);

    csv << "\nStream Type,Index,F#,HW Timestamp (ms),Host Timestamp(ms), 3DOF_x,3DOF_y,3DOF_z\n";

    for (const auto& elem : buffer)
    {
        for (auto i = 0; i < elem.second.size(); i++)
            csv << elem.second[i];
    }
}

enum application_stop : uint8_t {
    stop_on_frame_num,
    stop_on_timeout,
    stop_on_user_frame_num,
    stop_on_any
};

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

    std::cout << "Running data collection tool with the following params : ";
    for (auto i=1; i < argc; ++i)
        std::cout << argv[i] << " ";
    std::cout << std::endl;

    std::string output_file = filename.isSet() ? filename.getValue() : "frames_data.csv";

    auto max_frames_number = (max_frames.isSet()) ? max_frames.getValue() :MAX_FRAMES_NUMBER;

    auto stop_cond = static_cast<application_stop>((max_frames.isSet() << 1) | timeout.isSet());

    bool succeed = false;
    rs2::context ctx;
    rs2::device_list list;
    std::vector<rs2::sensor> active_sensors;

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

        orig_requests = requests;
        std::vector<rs2::stream_profile> matches, total_matches;
        // Configure and starts streaming
        for (auto&& sensor : dev->query_sensors())
        {
            for (auto& profile : sensor.get_stream_profiles())
            {
                // All requests have been resolved
                if (!requests.size())
                    break;

                // Find profile matches
                auto fulfilled_request = std::find_if(requests.begin(), requests.end(), [&matches, profile](const stream_request& req)
                {
                    bool res = false;
                    if (auto vp = profile.as<rs2::video_stream_profile>())
                    {
                        if ((vp.stream_type() == req._stream_type) &&
                            (vp.format() == req._stream_format) &&
                            (vp.stream_index() == req._stream_idx) &&
                            (vp.fps() == req._fps) &&
                            (vp.width() == req._width) &&
                            (vp.height() == req._height))
                        {
                            matches.push_back(profile);
                            res = true;
                        }
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
                active_sensors.emplace_back(sensor);
                matches.clear();
            }

            if (total_matches.size() == orig_requests.size())
                succeed = true;
        }

        std::cout << "Device selected: " << dev->get_info(RS2_CAMERA_INFO_NAME)
                << " s.n: " << dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)
                << "\nMatching profiles: " << total_matches.size() << std::endl << std::endl;

        std::map<std::pair<rs2_stream, int>, std::vector<frame_data>> buffer;
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
                    threedof_data data{ f.get_frame_number(),
                        f.get_timestamp(),
                        arrival_time.count(),
                        f.get_frame_timestamp_domain(),
                        f.get_profile().stream_type(),
                        f.get_profile().stream_index(),
                        axes.x, axes.y, axes.z};
                    buffer[stream_uid].push_back(data);
                }
            }
            else
            {
                if (buffer[stream_uid].size() < max_frames_number)
                {
                    buffer[stream_uid].push_back({ f.get_frame_number(),
                            f.get_timestamp(),
                            arrival_time.count(),
                            f.get_frame_timestamp_domain(),
                            f.get_profile().stream_type(),
                            f.get_profile().stream_index()});
                }
            }
        });
        std::cout << "Data collect started.... " << std::endl;

        const auto collection_accomplished = [&]()
        {
            //If none of the (timeout/num of frames) switches is set, terminate after max_frames_number.
            //If both switches are set, then terminate of whatever comes first
            //Otherwise, ready according to the switch specified
            bool timed_out = false;
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

        for (auto&& sensor : dev->query_sensors())
            sensor.stop();

        std::cout << "Data collection completed, serializing data to " << output_file << std::endl;
        save_data_to_file(buffer, total_matches, output_file);

        succeed = true;
    }
    std::cout << "Job completed" << std::endl;

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
