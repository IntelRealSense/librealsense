// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs.hpp>
#include <librealsense/rsutil.hpp>
#include "example.hpp"
#include <chrono>
#include <thread>
#include <iostream>
#include <fstream>
#include <sstream>
#include "tclap/CmdLine.h"


//using namespace std;
using namespace TCLAP;

const int MAX_FRAMES_NUMBER = 100;
const int NUM_OF_STREAMS = 9;

#define FROM_STRING(T,S,x)  for (int i = RS_##T##_ANY; i < RS_##T##_COUNT; i++) \
                            {\
                                if (rs_##S##_to_string((rs_##S##)i) == x)\
                                    return (rs_##S##)i;\
                            }\
                            return RS_##T##_ANY\

struct frame_data
{
    unsigned long long frame_number;
    double ts;
    long long arrival_time;
    rs_stream strean_type;
};

rs_stream from_string_to_stream(std::string t)
{
    FROM_STRING(STREAM, stream, t);
}
rs_format from_string_to_format(std::string f)
{
    FROM_STRING(FORMAT, format, f);
}
std::string toUpperCase(std::string s)
{
    char it[10];
    strncpy_s(it, s.c_str(), sizeof(it));
    it[sizeof(it) - 1] = 0;
    for (int i = 0; i < sizeof(it) - 1; i++)
    {
        if (it[i] >= 65 && it[i] <= 90)
            continue;
        if (it[i] >= 97 && it[i] <= 122)
        {
            it[i] -= 32;
        }
    }
    return std::string(it);
}

std::string get_log_level_string(rs_log_severity lvl)
{
    #define CASE(X) case RS_LOG_SEVERITY_##X: return #X;
    switch (lvl)
        {
            CASE(DEBUG)
            CASE(NONE)
            CASE(WARN)
            CASE(ERROR)
            CASE(FATAL)
            CASE(INFO)
        }
    #undef CASE
    return "";
}
rs_log_severity get_log_level(std::string str_lvl)
{
    rs_log_severity lvl;
    for (int level = 0; level< (int)RS_LOG_SEVERITY_COUNT; level++)
    {
        lvl = (rs_log_severity)level;
        if (get_log_level_string(lvl) == str_lvl)
            return lvl;
    }
    return RS_LOG_SEVERITY_COUNT;
}

rs::util::config configure_stream(bool is_file_set, std::string fn = "")
{
    rs::util::config config;

    if (!is_file_set)
    {
        config.enable_all(rs::preset::best_quality);
        std::cout << "Warning - No .csv configure file was passed, All streams are enabled by default.\n";
        std::cout << "To configure stream from a .csv file, specify file full path.\n";
        std::cout << "Verify stream requests are in the next format ([*] = cell):\n";
        std::cout << "[Stream Type] [Width] [Height] [FPS] [Format]\n";
        return config;
    }

    std::ifstream file("C:\\Users\\nircohen\\Desktop\\config.csv");
    //std::ifstream file(fn);
    
    std::string line;
    while (getline(file, line))
    {
        // Parsing configuration requests
        std::stringstream ss(line);
        std::vector<std::string> row;
        while (ss.good())
        {
            std::string substr;
            getline(ss, substr, ',');
            row.push_back(substr);
        }

        auto stream_type = from_string_to_stream(toUpperCase(row[0]));
        auto width = stoi(row[1]);
        auto height = stoi(row[2]);
        auto fps = stoi(row[3]);
        auto format = from_string_to_format(row[4]);

        // Configure
        config.enable_stream(stream_type, width, height, fps, format);
    }

    return config;
}

void save_data_to_file(std::list<frame_data> buffer[], std::string filename = ".\\frames_data.csv")
{
    // Save to file
    std::ofstream csv;
    csv.open(filename);
    csv << "Stream Type,F#,Timestamp,Arrival Time\n";
    
    for (int stream_index = 0; stream_index < NUM_OF_STREAMS; stream_index++)
    {
        for (unsigned int i = 0; i < buffer[stream_index].size(); i++)
        {
            std::ostringstream line;
            auto data = buffer[stream_index].front();
            line << rs_stream_to_string(data.strean_type) << "," << data.frame_number << "," << data.ts << "," << data.arrival_time << "\n";
            buffer[stream_index].pop_front();
            csv << line.str();
        }
    }
}

int main(int argc, char** argv) try
{

    // Parse command line arguments
    CmdLine cmd("librealsense cpp-data-collect example tool", ' ');
    ValueArg<int>         timeout    ("t", "Timeout",            "max amount of time to receive frames",                false, 10, "");
    ValueArg<std::string> max_frames ("m", "MaxFrames_Number",   "maximun number of frames data to receive",            false, "", "");
    ValueArg<std::string> filename   ("f", "FullFilePath",       "the file which the data will be saved to",            false, "", "");
    ValueArg<std::string> config_file("c", "ConfigurationFile",  "Specify file path with the requested configuration",  false, "", "");
    ValueArg<std::string> log_level  ("l", "LogLevel",           "Specify requested logging level",                     false, "", "");
    
    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(filename);
    cmd.add(config_file);
    cmd.add(log_level);
    cmd.parse(argc, argv);

    // Set console log level
    if (log_level.isSet())
    {
        rs_error * e = nullptr;
        rs_log_to_console(get_log_level(log_level.getValue()), &e);
        // Add log to file
    }


    rs::context ctx;
    
    auto list = ctx.query_devices();
    if (list.size() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    auto dev = list.front();
 
    // configure Streams
    auto is_file_set = filename.isSet();
    auto config = is_file_set ? configure_stream(true, config_file.getValue()) : configure_stream(false);
    auto stream = config.open(dev);

    std::list<frame_data> buffer[NUM_OF_STREAMS];
    auto start_time = std::chrono::high_resolution_clock::now();;

    stream.start([&buffer, &start_time](rs::frame f)
    {
        auto arrival_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);

        frame_data data;
        data.frame_number = f.get_frame_number();
        data.strean_type = f.get_stream_type();
        data.ts = f.get_timestamp();
        data.arrival_time = arrival_time.count();

        if (buffer[(int)data.strean_type].size() < MAX_FRAMES_NUMBER)
            buffer[(int)data.strean_type].push_back(data);
    });

    std::this_thread::sleep_for(std::chrono::seconds(timeout.getValue()));
   
    stream.stop();
   
    save_data_to_file(buffer);

    return EXIT_SUCCESS;
}
catch (const rs::error & e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception & e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
