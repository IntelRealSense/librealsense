// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <iostream>
#include <iomanip>
#include <core/streaming.h> //TODO: remove
#include <sensor.h>
#include <record_device.h>

#include "tclap/CmdLine.h"

using namespace std;
using namespace TCLAP;
using namespace rs2;

int main(int argc, const char** argv) try
{
    CmdLine cmd("librealsense cpp-record example", ' ', RS2_API_VERSION_STR);
    ValueArg<std::string> file_path("f", "file_path", "File path for recording output", false, "record_example.bag", "string");
    cmd.add( file_path );
    cmd.parse(argc, argv);

    context ctx;
    for(auto&& device : ctx.query_devices())
    {
        recorder record_device(file_path.getValue(), device);
    }



    std::cout << "Recording to file: " << file_path.getValue() << std::endl;
    return 0;
}
catch (const rs2::error & e)
{
    cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
    return EXIT_FAILURE;
}