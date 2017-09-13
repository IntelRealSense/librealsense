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

using namespace std;
using namespace TCLAP;
using namespace rs2;

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

rs2_format parse_format(const string str)
{
    for (int i = RS2_FORMAT_ANY; i < RS2_FORMAT_COUNT; i++)
    {
        if (rs2_format_to_string((rs2_format)i) == str)
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
        if (rs2_stream_to_string((rs2_stream)i) == str)
        {
            return (rs2_stream)i;
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
    transform(stream_type_str.begin(), stream_type_str.end(), stream_type_str.begin(), ptr_fun<int, int>(toupper));

    type    = parse_stream_type(stream_type_str);
    width   = parse_number(row[RES_WIDTH].c_str());
    height  = parse_number(row[RES_HEIGHT].c_str());
    fps     = parse_fps(row[FPS]);
    format  = parse_format(row[FORMAT]);
}

void configure_stream(pipeline& pipe, bool is_file_set, bool& is_valid, string fn = "")
{
    if (!is_file_set)
    {
        cout << " Warning - No .csv configure file was passed, All streams are enabled by default.\n";
        cout << " To configure stream from a .csv file, specify file full path.\n";
        cout << " Verify stream requests are in the next format -\n";
        cout << " [Stream Type] [Width] [Height] [FPS] [Format]\n";
    }

    ifstream file(fn);

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
        pipe.enable_stream(stream_type, 0, width, height, format, fps);
    }
}

void save_data_to_file(list<frame_data> buffer[], string filename = ".\\frames_data.csv")
{
    // Save to file
    ofstream csv;
    csv.open(filename);
    csv << "Stream Type,F#,Timestamp,Arrival Time\n";

    for (int stream_index = 0; stream_index < NUM_OF_STREAMS; stream_index++)
    {
        unsigned int buffer_size = buffer[stream_index].size();
        for (unsigned int i = 0; i < buffer_size; i++)
        {
            ostringstream line;
            auto data = buffer[stream_index].front();
            line << rs2_stream_to_string(data.stream_type) << "," << data.frame_number << "," << data.ts << "," << data.arrival_time << "\n";
            buffer[stream_index].pop_front();
            csv << line.str();
        }
    }

    csv.close();
}

int main(int argc, char** argv)
{
    log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense cpp-data-collect example tool", ' ');
    ValueArg<int>    timeout    ("t", "Timeout",            "Max amount of time to receive frames",                false, 10,  "");
    ValueArg<int>    max_frames ("m", "MaxFrames_Number",   "Maximun number of frames data to receive",            false, 100, "");
    ValueArg<string> filename   ("f", "FullFilePath",       "the file which the data will be saved to",            false, "",  "");
    ValueArg<string> config_file("c", "ConfigurationFile",  "Specify file path with the requested configuration",  false, "",  "");

    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(filename);
    cmd.add(config_file);
    cmd.parse(argc, argv);

    auto max_frames_number =  MAX_FRAMES_NUMBER;

    if (max_frames.isSet())
        max_frames_number = max_frames.getValue();

    bool succeed = false;
    std::mutex mutex;

    std::condition_variable cv;


    while(!succeed)
    {
        try
        {
            pipeline pipe;

            auto dev = pipe.get_device();

            bool need_to_reset = false;


            for (auto sub : dev.query_sensors())
            {
                sub.set_notifications_callback([&](const notification& n)
                {
                    if(n.get_category() == RS2_NOTIFICATION_CATEGORY_FRAMES_TIMEOUT)
                    {
                        std::unique_lock<std::mutex> lock(mutex);
                        need_to_reset = true;
                        cv.notify_one();
                    }
                });
            }


            // configure Streams
            bool is_valid_config;
            auto is_file_set = config_file.isSet();

            is_file_set ? configure_stream(pipe, true, is_valid_config, config_file.getValue()) : configure_stream(pipe, false, is_valid_config);
            

            std::list<frame_data> buffer[NUM_OF_STREAMS];
            auto start_time = chrono::high_resolution_clock::now();

            auto f = pipe.wait_for_frames();

            auto arrival_time = chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - start_time);

            frame_data data;
            data.frame_number = f.get_frame_number();
            data.stream_type = f.get_profile().stream_type();
            data.ts = f.get_timestamp();
            data.domain = f.get_frame_timestamp_domain();
            data.arrival_time = arrival_time.count();

            if (buffer[(int)data.stream_type].size() < max_frames_number)
                buffer[(int)data.stream_type].push_back(data);
            else
                cv.notify_one();


            const auto ready = [&]()
            {
                if(need_to_reset)
                    return true;

                for(auto&& profile : pipe.get_active_streams())
                {
                    if(buffer[(int) profile.stream_type()].size() < max_frames_number)
                        return false;
                }
                succeed = true;
                return true;
            };


            std::unique_lock<std::mutex> lock(mutex);
            auto res = cv.wait_for(lock,std::chrono::seconds(60), ready);
            if(res)
            {
                if(need_to_reset)
                {
                    succeed = false;
                    pipe.stop();
                    dev.hardware_reset();
                    need_to_reset = false;
                }
                if(!succeed && need_to_reset)
                    continue;
            }

            pipe.stop();
            if (filename.isSet())
                save_data_to_file(buffer, filename.getValue());
            else
                save_data_to_file(buffer);
        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
            return EXIT_FAILURE;
        }
        catch (const exception & e)
        {
            cerr << e.what() << endl;
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

