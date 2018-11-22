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

        virtual std::string to_string() const
        {
            std::stringstream ss;
            ss  << std::endl
                <<  rs2_stream_to_string(_stream_type) << ","
                << _stream_idx << "," << _frame_number << ","
                << std::fixed << std::setprecision(3) << _ts << "," << _arrival_time;

            return ss.str().c_str();
        }

        unsigned long long      _frame_number;
        double                  _ts;                // Device-based timestamp. (msec).
        double                  _arrival_time;      // Host arrival timestamp, relative to start streaming (msec)
        rs2_timestamp_domain    _domain;            // The origin of device-based timestamp. Note that Device timestamps may require kernel patches
        rs2_stream              _stream_type;
        int                     _stream_idx;
    };

    // Generic 3DoF data. The values and units are sensor-specific
    struct threedof_data : frame_data
    {
        threedof_data(unsigned long long frame_number,
                   double               frame_ts,
                   double               host_ts,
                   rs2_timestamp_domain domain,
                   rs2_stream           stream_type,
                   int                  stream_index,
                   double x, double y, double z):
            frame_data(frame_number, frame_ts, host_ts, domain, stream_type, stream_index),
            _x_axis(x), _y_axis(y), _z_axis(z)
            {};

        virtual std::string to_string() const override
        {
            std::stringstream ss;
            ss  << frame_data::to_string()<< ","
                << _x_axis << "," << _y_axis << "," << _z_axis;

            return ss.str().c_str();
        }

        double  _x_axis;
        double  _y_axis;
        double  _z_axis;
    };

    // Explicit 6DOF positioning data
    struct sixdof_data : frame_data
    {
        sixdof_data(unsigned long long frame_number,
                   double frame_ts,
                   double host_ts,
                   rs2_timestamp_domain domain,
                   rs2_stream stream_type,
                   int stream_index,
                   double t_x, double t_y, double t_z, double r_x, double r_y, double r_z, double r_w):
            frame_data(frame_number, frame_ts, host_ts, domain, stream_type, stream_index),
            _trans_x(t_x), _trans_y(t_y), _trans_z(t_z),
            _rot_x(r_x), _rot_y(r_y), _rot_z(r_z),_rot_w(r_w)
            {};

        virtual std::string to_string() const override
        {
            std::stringstream ss;
            ss  << frame_data::to_string()<< "," << std::fixed
                << _trans_x << "," << _trans_y << "," << _trans_z << ","
                << _rot_x << "," << _rot_y << "," << _rot_z << "," << _rot_w;

            return ss.str().c_str();
        }

        // Translation and rotation (quaternion) data
        double  _trans_x;
        double  _trans_y;
        double  _trans_z;
        double  _rot_x;
        double  _rot_y;
        double  _rot_z;
        double  _rot_w;
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
        return os << "Type: " << req._stream_type << ", Idx: " << req._stream_idx << ", "
            << req._stream_format << ", [" << req._width << "x" << req._height << "], " << req._fps << "fps" << std::endl;
    }

    using data_collection = std::map<std::pair<rs2_stream, int>, std::vector<std::shared_ptr<frame_data>>>;

    enum config_params { STREAM_TYPE = 0, RES_WIDTH, RES_HEIGHT, FPS, FORMAT, STREAM_INDEX };

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

    void save_data_to_file(const data_collection& buffer,
                            const std::vector<rs2::stream_profile>& active_stream_profiles,
                            const string& out_filename)
    {
        ofstream csv(out_filename);

        csv << "Configuration:\nStream Type,Stream Name,Format,FPS,Width,Height\n";
        for (const auto& elem : active_stream_profiles)
            csv << get_profile_description(elem);

        for (const auto& elem : buffer)
        {
            csv << "\n\nStream Type,Index,F#,HW Timestamp (ms),Host Timestamp(ms)"
                << (val_in_range(elem.first.first, {RS2_STREAM_GYRO,RS2_STREAM_ACCEL}) ? ",3DOF_x,3DOF_y,3DOF_z" : "")
                << (val_in_range(elem.first.first, {RS2_STREAM_POSE}) ? ",t_x,t_y,t_z,r_x,r_y,r_z,r_w" : "")
                << std::endl;

            for (auto i = 0; i < elem.second.size(); i++)
                csv << elem.second[i]->to_string();
        }
    }
}
