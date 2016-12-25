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
#include <condition_variable>
#include <set>
#include <cctype>

//using namespace std;
using namespace TCLAP;

int MAX_FRAMES_NUMBER = 100;
const unsigned int NUM_OF_STREAMS = (int)RS_STREAM_COUNT;

struct frame_data
{
    unsigned long long frame_number;
    double ts;
    long long arrival_time;
    rs_timestamp_domain domain;
    rs_stream strean_type;
};

enum Config_Params { STREAM_TYPE = 0, RES_WIDTH, RES_HEIGHT, FPS, FORMAT };

int parse_number(char const *s, int base = 0)
{
    char c;
    std::stringstream ss(s);
    int i;
    ss >> i;
    
    if (ss.fail() || ss.get(c)) 
    {
        throw std::runtime_error(std::string(std::string("Invalid numeric input - ") + s + std::string("\n")));
    }
    return i;
}

rs_format parse_format(const std::string str)
{
    for (int i = RS_FORMAT_ANY; i < RS_FORMAT_COUNT; i++)
    {
        if (rs_format_to_string((rs_format)i) == str)
        {
            return (rs_format)i;
        }
    }
    throw std::runtime_error((std::string("Invalid format - ") + str + std::string("\n")).c_str());
}

rs_stream parse_stream_type(const std::string str)
{
    for (int i = RS_STREAM_ANY; i < RS_STREAM_COUNT; i++)
    {
        if (rs_stream_to_string((rs_stream)i) == str)
        {
            return (rs_stream)i;
        }
    }
    throw std::runtime_error((std::string("Invalid stream type - ") + str + std::string("\n")).c_str());
}

int parse_fps(const std::string str)
{
    std::set<int> valid_fps({ 10, 15, 30, 60, 90, 100, 200 });
    auto fps = parse_number(str.c_str());
    if (valid_fps.find(fps) == valid_fps.end())
    {
        throw std::runtime_error( (std::string("Invalid FPS parameter - ") + str + std::string("\n")).c_str());
    }
    return fps;
}

void parse_configuration(const std::vector<std::string> row, rs_stream& type, int& width, int& height, rs_format& format, int& fps)
{
    // Convert string to uppercase
    auto stream_type_str = row[STREAM_TYPE];
    std::transform(stream_type_str.begin(), stream_type_str.end(), stream_type_str.begin(), std::ptr_fun<int, int>(std::toupper));

    type    = parse_stream_type(stream_type_str);
    width   = parse_number(row[RES_WIDTH].c_str());
    height  = parse_number(row[RES_HEIGHT].c_str());
    fps     = parse_fps(row[FPS]);
    format  = parse_format(row[FORMAT]);
}

rs::util::config configure_stream(bool is_file_set, bool& is_valid, std::string fn = "")
{
    rs::util::config config;

    if (!is_file_set)
    {
        config.enable_all(rs::preset::best_quality);
        std::cout << " Warning - No .csv configure file was passed, All streams are enabled by default.\n";
        std::cout << " To configure stream from a .csv file, specify file full path.\n";
        std::cout << " Verify stream requests are in the next format -\n";
        std::cout << " [Stream Type] [Width] [Height] [FPS] [Format]\n";
        return config;
    }

    std::ifstream file(fn);

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

        rs_stream stream_type;
        rs_format format;
        int width, height, fps;

        // correctness check
        parse_configuration(row, stream_type, width, height, format, fps);
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
    ValueArg<int> timeout            ("t", "Timeout",            "Max amount of time to receive frames",                false, 10,  "");
    ValueArg<int> max_frames         ("m", "MaxFrames_Number",   "Maximun number of frames data to receive",            false, 100, "");
    ValueArg<std::string> filename   ("f", "FullFilePath",       "the file which the data will be saved to",            false, "",  "");
    ValueArg<std::string> config_file("c", "ConfigurationFile",  "Specify file path with the requested configuration",  false, "",  "");
    
    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(filename);
    cmd.add(config_file);
    cmd.parse(argc, argv);

    if (max_frames.isSet())
        MAX_FRAMES_NUMBER = max_frames.getValue();


    rs::context ctx;
    
    auto list = ctx.query_devices();
    if (list.size() == 0) throw std::runtime_error("No device detected. Is it plugged in?");
    auto dev = list.front();
 
    // configure Streams
    bool is_valid_config;
    auto is_file_set = filename.isSet();
 
    auto config = is_file_set ? configure_stream(true, is_valid_config, config_file.getValue()) : configure_stream(false, is_valid_config);
    auto camera = config.open(dev);

    std::list<frame_data> buffer[NUM_OF_STREAMS];
    auto start_time = std::chrono::high_resolution_clock::now();

    std::condition_variable cv;

    camera.start([&buffer, &start_time](rs::frame f)
    {
        auto arrival_time = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time);

        frame_data data;
        data.frame_number = f.get_frame_number();
        data.strean_type = f.get_stream_type();
        data.ts = f.get_timestamp();
        data.domain = f.get_frame_timestamp_domain();
        data.arrival_time = arrival_time.count();

        if (buffer[(int)data.strean_type].size() < MAX_FRAMES_NUMBER)
            buffer[(int)data.strean_type].push_back(data);
    });

    std::this_thread::sleep_for(std::chrono::seconds(timeout.getValue()));
   
    camera.stop();
   
    if (filename.isSet())
        save_data_to_file(buffer, filename.getValue());
    else
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
