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
#include <atomic>

using namespace std;
using namespace TCLAP;

int MAX_FRAMES_NUMBER = 10; //number of frames to capture from each stream
const unsigned int NUM_OF_STREAMS = static_cast<int>(RS2_STREAM_COUNT);

struct frame_data
{
    unsigned long long frame_number;
    double ts;
    long long arrival_time;
    rs2_timestamp_domain domain;
    rs2_stream stream_type;
};

enum Config_Params { STREAM_TYPE = 0, RES_WIDTH, RES_HEIGHT, FPS, FORMAT };

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

void parse_configuration(const vector<string> row, rs2_stream& type, int& width, int& height, rs2_format& format, int& fps)
{
    // Convert string to uppercase
    auto stream_type_str = row[STREAM_TYPE];
    type = parse_stream_type(to_lower(stream_type_str));
    width = parse_number(row[RES_WIDTH].c_str());
    height = parse_number(row[RES_HEIGHT].c_str());
    fps = parse_fps(row[FPS]);
    format = parse_format(to_lower(row[FORMAT]));
}

void configure_stream(rs2::config& cfg, std::string filename)
{
    ifstream file(filename);

    if (!file.is_open())
        throw runtime_error("Given .csv configure file Not Found!");

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
        int width, height, fps;

        // correctness check
        parse_configuration(row, stream_type, width, height, format, fps);
        cfg.enable_stream(stream_type, 0, width, height, format, fps);
    }
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
    ValueArg<int>    max_frames("m", "MaxFrames_Number", "Maximun number of frames data to receive", false, 100, "");
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

    while (!succeed)
    {
        rs2::pipeline pipe;
        rs2::config config;
        if (config_file.isSet())
        {
            configure_stream(config, config_file.getValue());
        }

        rs2::pipeline_profile profile = pipe.start(config);
        auto dev = profile.get_device();

        std::atomic_bool need_to_reset(false);
        for (auto sub : dev.query_sensors())
        {
            sub.set_notifications_callback([&](const rs2::notification& n)
            {
                if (n.get_category() == RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT)
                {
                    need_to_reset = true;
                }
            });
        }

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

            bool collected_enough_frames = true;
            for (auto&& profile : pipe.get_active_profile().get_streams())
            {
                if (buffer[(int)profile.stream_type()].size() < max_frames_number)
                {
                    collected_enough_frames = false;
                }
            }

            if (timeout.isSet() && !max_frames.isSet())
                return timed_out;

            return timed_out || collected_enough_frames;
        };

        while (!ready())
        {
            auto frameset = pipe.wait_for_frames();
            auto arrival_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time);

            for (auto it = frameset.begin(); it != frameset.end(); ++it)
            {
                auto f = (*it);
                frame_data data{f.get_frame_number(),
                                f.get_timestamp(),
                                arrival_time.count(),
                                f.get_frame_timestamp_domain(),
                                f.get_profile().stream_type()};
                if (buffer[(int)data.stream_type].size() < max_frames_number)
                {
                    buffer[(int)data.stream_type].push_back(data);
                }
            }

            if (need_to_reset)
            {
                dev.hardware_reset();
                need_to_reset = false;
                break;
            }
        }

        if(ready())
        {
            save_data_to_file(buffer, output_file);
            pipe.stop();
            succeed = true;
        }
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
