// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.
// The utility shall be used to collect and analyze Realsense Cameras performance.
// Therefore no UI is provided, and the data is gathered and serialized in csv-compatible format for offline analysis.
// The utility is configured via command-line parameters and requires user-provided configuration file to run
// The following lines provide examples of the configurations available
/*
//Defines the order at which the params (comma-separated) should appear in configuration file
//#rs2_stream, width, height, fps, rs2_format, stream_index (optional, put zero or leave empty if you're not sure)
//# Examples (note that all lines starting with non-alpha character (#@!...) will be skipped:
#D435/415
DEPTH,640,480,30,Z16,0
INFRARED,640,480,30,Y8,1
INFRARED,640,480,30,Y8,2
COLOR,640,480,30,RGB8,0
##D435i-specific
ACCEL,1,1,63,MOTION_XYZ32F
GYRO,1,1,200,MOTION_XYZ32F
#T265
#FISHEYE,848,800,30,Y8,1
#FISHEYE,848,800,30,Y8,2
#ACCEL,1,1,62,MOTION_XYZ32F,0
#GYRO,1,1,200,MOTION_XYZ32F,0
#POSE,0,0,262,6DOF,0
*/

#include <librealsense2/rs.hpp>
#include "tclap/CmdLine.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>
#include <iomanip>
#include <map>
#include <atomic>
#include <regex>
#include <algorithm>    // std::sort

using namespace std;
using namespace TCLAP;

namespace rs_data_collect
{
    const uint64_t  DEF_FRAMES_NUMBER = 100;
    const std::string DEF_OUTPUT_FILE_NAME("frames_data.csv");

    // Split string into token,  trim unreadable characters
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

    template <typename T>
    bool val_in_range(const T& val, const std::initializer_list<T>& list)
    {
        for (const auto& i : list) {
            if (val == i) {
                return true;
            }
        }
        return false;
    }

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
        return os << "Type: " << req._stream_type << ", Idx: " << req._stream_idx << ", "
            << req._stream_format << ", [" << req._width << "x" << req._height << "], " << req._fps << "fps" << std::endl;
    }

    enum config_params { STREAM_TYPE = 0, RES_WIDTH, RES_HEIGHT, FPS, FORMAT, STREAM_INDEX };

    enum application_stop : uint8_t { stop_on_frame_num,
        stop_on_timeout,
        stop_on_user_frame_num,
        stop_on_any
    };

    class data_collector
    {
    public:
        data_collector(std::shared_ptr<rs2::device> dev,
        ValueArg<int>& timeout, ValueArg<int>& max_frames): _dev(dev)
            {
                _max_frames = max_frames.isSet() ? max_frames.getValue() :
                                timeout.isSet() ? std::numeric_limits<uint64_t>::max() : DEF_FRAMES_NUMBER;
                _time_out_sec = timeout.isSet() ? timeout.getValue(): -1;

                _stop_cond = static_cast<application_stop>((max_frames.isSet() << 1) | timeout.isSet());
            };
        ~data_collector(){};
        data_collector(const data_collector&);

        void parse_and_configure(ValueArg<string>& config_file)
        {
            if (!config_file.isSet())
                throw std::runtime_error(stringify()
                    << "The tool requires a profile configuration file to proceed"
                    << "\nRun rs-data-collect --help for details");

            const std::string config_filename(config_file.getValue());
            ifstream file(config_filename);

            if (!file.is_open())
                throw runtime_error(stringify() << "Given .csv configure file " << config_filename <<  " was not found!");

            // Line starting with non-alpha characters will be treated as comments
            const static std::regex starts_with("^[a-zA-Z]");
            string line;
            rs2_stream stream_type;
            rs2_format format;
            int width{}, height{}, fps{}, index{};

            // Parse the config file
            while (getline(file, line))
            {
                auto tokens =tokenize(line,',');

                if (std::regex_search(line, starts_with))
                {
                    if (parse_configuration(line, tokens, stream_type, width, height, format, fps,index))
                        user_requests.push_back({ stream_type, format, width, height, fps,  index });
                }
            }

            // Sanity test agains multiple conflicting requests for the same sensor
            if (user_requests.size())
            {
                std::sort(user_requests.begin(), user_requests.end(),
                    [](const stream_request& l, const stream_request& r){ return l._stream_type < r._stream_type; });

                for (auto i = 0; i < user_requests.size() - 1; i++)
                {
                    if ((user_requests[i]._stream_type == user_requests[i+1]._stream_type) && ((user_requests[i]._stream_idx == user_requests[i+1]._stream_idx)))
                        throw runtime_error(stringify() << "Invalid configuration file - multiple requests for the same sensor:\n"
                            << user_requests[i] << user_requests[+i]);
                }
            }
            else
                throw std::runtime_error(stringify() << "Invalid configuration file - " << config_filename  << " zero requests accepted");

            // Assign user profile to actual sensors
            configure_sensors();

            // Report results
            std::cout << "\nDevice selected: \n\t" << _dev->get_info(RS2_CAMERA_INFO_NAME)
                << (_dev->supports(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR) ?
                    std::string(( stringify() << ". USB Type: " << _dev->get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR))) : "")
                << "\n\tS.N: " << (_dev->supports(RS2_CAMERA_INFO_SERIAL_NUMBER) ? _dev->get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) : "")
                << "\n\tFW Ver: " << _dev->get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION)
                << "\nUser streams requested: " << user_requests.size()
                << ", actual matches: " << selected_stream_profiles.size() << std::endl;

            if (requests_to_go.size())
            {
                std::cout << "\nThe following request(s) are not supported by the device: " << std::endl;
                for(auto& elem : requests_to_go)
                    std::cout << elem;
            }
        }

        void save_data_to_file(const string& out_filename)
        {
            if (!data_collection.size())
                throw runtime_error(stringify() << "No data collected, aborting");

            // Report amount of frames collected
            std::vector<uint64_t> frames_per_stream;
            for (const auto& kv : data_collection)
                frames_per_stream.emplace_back(kv.second.size());

            std::sort(frames_per_stream.begin(),frames_per_stream.end());
            std::cout   << "\nData collection accomplished with ["
                        << frames_per_stream.front() << "-" << frames_per_stream.back()
                        << "] frames recorded per stream\nSerializing captured results to"
                        << out_filename << std::endl;

            // Serialize and store data into csv-like format
            ofstream csv(out_filename);
            if (!csv.is_open())
                throw runtime_error(stringify() << "Cannot open the requested output file " << out_filename <<  ", please check permissions");

            csv << "Configuration:\nStream Type,Stream Name,Format,FPS,Width,Height\n";
            for (const auto& elem : selected_stream_profiles)
                csv << get_profile_description(elem);

            for (const auto& elem : data_collection)
            {
                csv << "\n\nStream Type,Index,F#,HW Timestamp (ms),Host Timestamp(ms)"
                    << (val_in_range(elem.first.first, {RS2_STREAM_GYRO,RS2_STREAM_ACCEL}) ? ",3DOF_x,3DOF_y,3DOF_z" : "")
                    << (val_in_range(elem.first.first, {RS2_STREAM_POSE}) ? ",t_x,t_y,t_z,r_x,r_y,r_z,r_w" : "")
                    << std::endl;

                for (auto i = 0; i < elem.second.size(); i++)
                    csv << elem.second[i].to_string();
            }
        }

        void collect_frame_attributes(rs2::frame f, std::chrono::time_point<std::chrono::high_resolution_clock> start_time)
        {
            auto arrival_time = std::chrono::duration<double, std::milli>(chrono::high_resolution_clock::now() - start_time);
            auto stream_uid = std::make_pair(f.get_profile().stream_type(), f.get_profile().stream_index());

            if (data_collection[stream_uid].size() < _max_frames)
            {
                frame_record rec{f.get_frame_number(),
                    f.get_timestamp(),
                    arrival_time.count(),
                    f.get_frame_timestamp_domain(),
                    f.get_profile().stream_type(),
                    f.get_profile().stream_index()};

                if (auto motion = f.as<rs2::motion_frame>())
                {
                    auto axes = motion.get_motion_data();
                    rec._params = {axes.x, axes.y, axes.z};
                }

                if (auto pf = f.as<rs2::pose_frame>())
                {
                    auto pose = pf.get_pose_data();
                    rec._params = {pose.translation.x, pose.translation.y, pose.translation.z,
                            pose.rotation.x,pose.rotation.y,pose.rotation.z,pose.rotation.w};
                }

                data_collection[stream_uid].emplace_back(rec);
            }
        }

        bool collecting(std::chrono::time_point<std::chrono::high_resolution_clock> start_time)
        {
            bool timed_out = false;

            if (_time_out_sec>0)
            {
                timed_out = (chrono::high_resolution_clock::now() - start_time) > std::chrono::seconds(_time_out_sec);
                // When the timeout is the only option is specified, disregard frame number
                if (stop_on_timeout==_stop_cond)
                    return !timed_out;
            }

            bool collected_enough_frames = true;
            for (auto&& profile : selected_stream_profiles)
            {
                auto key = std::make_pair(profile.stream_type(),profile.stream_index());
                if (!data_collection.size() || (data_collection.find(key) !=data_collection.end() &&
                    (data_collection[key].size() && data_collection[key].size() < _max_frames)))
                {
                    collected_enough_frames = false;
                    break;
                }
            }

             return !(timed_out || collected_enough_frames);

        }

        const std::vector<rs2::sensor>& selected_sensors() const { return active_sensors; };

        struct frame_record
        {
            frame_record(unsigned long long frame_number, double frame_ts, double host_ts,
                       rs2_timestamp_domain domain, rs2_stream stream_type,int stream_index,
                       double _p1=0., double _p2=0., double _p3=0.,
                       double _p4=0., double _p5=0., double _p6=0., double _p7=0.):
            _frame_number(frame_number),
            _ts(frame_ts),
            _arrival_time(host_ts),
            _domain(domain),
            _stream_type(stream_type),
            _stream_idx(stream_index),
            _params({_p1,_p2,_p3,_p4,_p5,_p6,_p7})
            {};

            std::string to_string() const
            {
                std::stringstream ss;
                ss  << std::endl
                    <<  rs2_stream_to_string(_stream_type) << ","
                    << _stream_idx << "," << _frame_number << ","
                    << std::fixed << std::setprecision(3) << _ts << "," << _arrival_time;

                // IMU and Pose frame hold the sample data in addition to the frame's header attributes
                size_t specific_attributes = 0;
                if (val_in_range(_stream_type,{RS2_STREAM_GYRO,RS2_STREAM_ACCEL}))
                    specific_attributes = 3;
                if (val_in_range(_stream_type,{RS2_STREAM_POSE}))
                    specific_attributes = 7;

                    for (auto i=0; i<specific_attributes; i++)
                        ss << "," << _params[i];

                return ss.str().c_str();
            }

            unsigned long long      _frame_number;
            double                  _ts;                // Device-based timestamp. (msec).
            double                  _arrival_time;      // Host arrival timestamp, relative to start streaming (msec)
            rs2_timestamp_domain    _domain;            // The origin of device-based timestamp. Note that Device timestamps may require kernel patches
            rs2_stream              _stream_type;
            int                     _stream_idx;
            std::array<double,7>    _params;            // |The parameters are optional and sensor specific
        };

    private:

        std::shared_ptr<rs2::device>        _dev;
        std::map<std::pair<rs2_stream, int>, std::vector<frame_record>> data_collection;
        std::vector<stream_request>         requests_to_go, user_requests;
        std::vector<rs2::sensor>            active_sensors;
        std::vector<rs2::stream_profile>    selected_stream_profiles;
        uint64_t                            _max_frames;
        int64_t                             _time_out_sec;
        application_stop                    _stop_cond;

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

        // Assign the user configuration to the selected device
        bool configure_sensors()
        {
            bool succeed = false;
            requests_to_go = user_requests;
            std::vector<rs2::stream_profile> matches;

            // Configure and starts streaming
            for (auto&& sensor : _dev->query_sensors())
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
                    std::copy(matches.begin(), matches.end(), std::back_inserter(selected_stream_profiles));
                    sensor.open(matches);
                    active_sensors.emplace_back(sensor);
                    matches.clear();
                }

                if (selected_stream_profiles.size() == user_requests.size())
                    succeed = true;
            }
            return succeed;
        }
    };
}
