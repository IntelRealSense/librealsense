// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "librealsense2/rs.hpp"

#include "tclap/CmdLine.h"

#include "converters/converter-csv.h"
#include "converters/converter-png.h"


using namespace std;
using namespace TCLAP;


int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-convert tool", ' ');
    ValueArg<string> inputFilename("i", "InputFilePath", "ROS-bag filename", true, "", "");
    ValueArg<string> outputFilenamePng("p", "OutputPngFilePath", "PNG filename", false, "", "");
    ValueArg<string> outputFilenameCsv("c", "OutputCsvFilePath", "CSV filename", false, "", "");

    cmd.add(inputFilename);
    cmd.add(outputFilenamePng);
    cmd.add(outputFilenameCsv);
    cmd.parse(argc, argv);

    shared_ptr<rs2::tools::converter::Converter> converter;

    if (outputFilenameCsv.isSet()) {
        converter.reset(new rs2::tools::converter::ConverterCsv());
    }
    else if (outputFilenamePng.isSet()) {
        converter.reset(new rs2::tools::converter::ConverterPng(outputFilenamePng.getValue()));
    }
    else {
        throw exception("output not defined");
    }

    auto pipe = make_shared<rs2::pipeline>();
    rs2::config cfg;
    cfg.enable_device_from_file(inputFilename.getValue());
    pipe->start(cfg);

    rs2::frameset frameset;
    unsigned long long frameNumber = 0;

    while (true) {
        frameset = pipe->wait_for_frames();

        // any better method to check for the last frame?
        if (frameset[0].get_frame_number() < frameNumber) {
            break;
        }

        converter->convert(frameset);

        frameNumber = frameset[0].get_frame_number();
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
