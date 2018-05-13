// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "librealsense2/rs.hpp"

#include "tclap/CmdLine.h"

#include "converters/converter-csv.h"


using namespace std;
using namespace TCLAP;


int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-convert tool", ' ');
    ValueArg<string> inputFilename("i", "InputFilePath", "ROS-bag filename", true, "", "");

    cmd.add(inputFilename);
    cmd.parse(argc, argv);

    shared_ptr<rs2::converter::Converter> converter;

    converter.reset(new rs2::converter::ConverterCsv());

    auto pipe = make_shared<rs2::pipeline>();
    rs2::config cfg;
    cfg.enable_device_from_file(inputFilename.getValue());
    pipe->start(cfg);

    auto frames = pipe->wait_for_frames();
    converter->convert(frames);

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
