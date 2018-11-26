// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.
// Minimalistic command-line collect & analyze bandwidth/performance tool for Realsense Cameras.
// The data is gathered and serialized in csv-compatible format for offline analysis.
// Extract and store frame headers info for video streams; for IMU&Tracking streams also store the actual data
// The utility is configured with command-line keys and requires user-provided config file to run
// See rs-data-collect.h for config examples

#include <librealsense2/rs.hpp>
#include "rs-data-collect.h"
#include <thread>
#include <iostream>

using namespace std;
using namespace TCLAP;

using namespace rs_data_collect;

int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-data-collect example tool", ' ');
    ValueArg<int>    timeout("t", "Timeout", "Max amount of time to receive frames (in seconds)", false, 10, "");
    ValueArg<int>    max_frames("m", "MaxFrames_Number", "Maximum number of frames-per-stream to receive", false, 100, "");
    ValueArg<string> out_file("f", "FullFilePath", "the file where the data will be saved to", false, "", "");
    ValueArg<string> config_file("c", "ConfigurationFile", "Specify file path with the requested configuration", false, "", "");

    cmd.add(timeout);
    cmd.add(max_frames);
    cmd.add(out_file);
    cmd.add(config_file);
    cmd.parse(argc, argv);

    std::cout << "Running rs-data-collect: ";
    for (auto i=1; i < argc; ++i)
        std::cout << argv[i] << " ";
    std::cout << std::endl;

    auto output_file       = out_file.isSet() ? out_file.getValue() : DEF_OUTPUT_FILE_NAME;
    auto max_frames_number = max_frames.isSet() ? max_frames.getValue() :
                                timeout.isSet() ? std::numeric_limits<uint64_t>::max() : DEF_FRAMES_NUMBER;

    auto stop_cond = static_cast<application_stop>((max_frames.isSet() << 1) | timeout.isSet());

    bool succeed = false;
    rs2::context ctx;
    rs2::device_list list;

    while (!succeed)
    {
        list = ctx.query_devices();

        if (0== list.size())
        {
            std::cout << "Connect Realsense Camera to proceed" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(3));
            continue;
        }

        auto dev = std::make_shared<rs2::device>(list.front());

        data_collector  dc(dev,timeout,max_frames);         // Parser and the data aggregator

        dc.parse_and_configure(config_file);

        //data_collection buffer;
        auto start_time = chrono::high_resolution_clock::now();

        // Start streaming
        for (auto&& sensor : dc.selected_sensors())
            sensor.start([&dc,&start_time](rs2::frame f)
        {
            dc.collect_frame_attributes(f,start_time);
        });

        std::cout << "\nData collection started.... \n" << std::endl;

        //while (!collection_accomplished())
        while (dc.collecting(start_time))
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << "Collecting data for "
                    << chrono::duration_cast<chrono::seconds>(chrono::high_resolution_clock::now() - start_time).count()
                    << " sec" << std::endl;
        }

        // Stop & flush all active sensors
        for (auto&& sensor : dc.selected_sensors())
        {
            sensor.stop();
            sensor.close();
        }

        dc.save_data_to_file(output_file);

        succeed = true;
    }

    std::cout << "Task completed" << std::endl;

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
