// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "librealsense2/rs.hpp"

#include "tclap/CmdLine.h"

#include "converters/converter-csv.hpp"
#include "converters/converter-png.hpp"


using namespace std;
using namespace TCLAP;


int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-convert tool", ' ');
    ValueArg<string> inputFilename("i", "input", "ROS-bag filename", true, "", "ros-bag-file");
    ValueArg<string> outputFilenamePng("p", "output-png", "output PNG file(s) path", false, "", "png-path");
    ValueArg<string> outputFilenameCsv("c", "output-csv", "output CSV file(s) path", false, "", "csv-path");

    cmd.add(inputFilename);
    cmd.add(outputFilenamePng);
    cmd.add(outputFilenameCsv);
    cmd.parse(argc, argv);

    vector<shared_ptr<rs2::tools::converter::converter_base>> converters;

    if (outputFilenameCsv.isSet()) {
        converters.push_back(make_shared<rs2::tools::converter::converter_csv>());
    }

    if (outputFilenamePng.isSet()) {
        converters.push_back(make_shared<rs2::tools::converter::converter_png>(rs2_stream::RS2_STREAM_DEPTH, outputFilenamePng.getValue()));
    }

    if (converters.empty()) {
        throw exception("output not defined");
    }

    auto pipe = make_shared<rs2::pipeline>();
    rs2::config cfg;
    cfg.enable_device_from_file(inputFilename.getValue());
    pipe->start(cfg);
    auto device = pipe->get_active_profile().get_device();
    rs2::playback playback = device.as<rs2::playback>();
    playback.set_real_time(false);

    rs2::frameset frameset;
    auto frameNumber = 0ULL;

    while (true) {
        frameset = pipe->wait_for_frames();
        playback.pause();

        // any better method to check for the last frame?
        if (frameset[0].get_frame_number() < frameNumber) {
            break;
        }

        frameNumber = frameset[0].get_frame_number();

        for_each(converters.begin(), converters.end(),
            [&frameset] (shared_ptr<rs2::tools::converter::converter_base>& converter) {
                converter->convert(frameset);
            });

        for_each(converters.begin(), converters.end(),
            [&frameset] (shared_ptr<rs2::tools::converter::converter_base>& converter) {
                converter->wait();
            });

        playback.resume();
    }

    return EXIT_SUCCESS;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function()
        << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;

    return EXIT_FAILURE;
}
catch (const exception & e)
{
    cerr << e.what() << endl;
    return EXIT_FAILURE;
}
catch (...)
{
    cerr << "some error" << endl;
    return EXIT_FAILURE;
}
