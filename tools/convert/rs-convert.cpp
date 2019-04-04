// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2018 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "librealsense2/rs.hpp"

#include "tclap/CmdLine.h"

#include "converters/converter-csv.hpp"
#include "converters/converter-png.hpp"
#include "converters/converter-raw.hpp"
#include "converters/converter-ply.hpp"
#include "converters/converter-bin.hpp"


using namespace std;
using namespace TCLAP;


int main(int argc, char** argv) try
{
    rs2::log_to_file(RS2_LOG_SEVERITY_WARN);

    // Parse command line arguments
    CmdLine cmd("librealsense rs-convert tool", ' ');
    ValueArg<string> inputFilename("i", "input", "ROS-bag filename", true, "", "ros-bag-file");
    ValueArg<string> outputFilenamePng("p", "output-png", "output PNG file(s) path", false, "", "png-path");
    ValueArg<string> outputFilenameCsv("v", "output-csv", "output CSV (depth matrix) file(s) path", false, "", "csv-path");
    ValueArg<string> outputFilenameRaw("r", "output-raw", "output RAW file(s) path", false, "", "raw-path");
    ValueArg<string> outputFilenamePly("l", "output-ply", "output PLY file(s) path", false, "", "ply-path");
    ValueArg<string> outputFilenameBin("b", "output-bin", "output BIN (depth matrix) file(s) path", false, "", "bin-path");
    SwitchArg switchDepth("d", "depth", "convert depth frames (default - all supported)", false);
    SwitchArg switchColor("c", "color", "convert color frames (default - all supported)", false);

    cmd.add(inputFilename);
    cmd.add(outputFilenamePng);
    cmd.add(outputFilenameCsv);
    cmd.add(outputFilenameRaw);
    cmd.add(outputFilenamePly);
    cmd.add(outputFilenameBin);
    cmd.add(switchDepth);
    cmd.add(switchColor);
    cmd.parse(argc, argv);

    vector<shared_ptr<rs2::tools::converter::converter_base>> converters;

    rs2_stream streamType = switchDepth.isSet() ? rs2_stream::RS2_STREAM_DEPTH
        : switchColor.isSet() ? rs2_stream::RS2_STREAM_COLOR
        : rs2_stream::RS2_STREAM_ANY;

    if (outputFilenameCsv.isSet()) {
        converters.push_back(
            make_shared<rs2::tools::converter::converter_csv>(
                outputFilenameCsv.getValue()
                , streamType));
    }

    if (outputFilenamePng.isSet()) {
        converters.push_back(
            make_shared<rs2::tools::converter::converter_png>(
                outputFilenamePng.getValue()
                , streamType));
    }

    if (outputFilenameRaw.isSet()) {
        converters.push_back(
            make_shared<rs2::tools::converter::converter_raw>(
                outputFilenameRaw.getValue()
                , streamType));
    }

    if (outputFilenamePly.isSet()) {
        converters.push_back(
            make_shared<rs2::tools::converter::converter_ply>(
                outputFilenamePly.getValue()));
    }

    if (outputFilenameBin.isSet()) {
        converters.push_back(
            make_shared<rs2::tools::converter::converter_bin>(
                outputFilenameBin.getValue()));
    }

    if (converters.empty()) {
        throw runtime_error("output not defined");
    }

    // Since we are running in blocking "non-real-time" mode,
    // we don't want to prevent process termination if some of the frames
    // did not find a match and hence were not serviced
    auto pipe = std::shared_ptr<rs2::pipeline>(
        new rs2::pipeline(), [](rs2::pipeline*) {});

    rs2::config cfg;
    cfg.enable_device_from_file(inputFilename.getValue());
    pipe->start(cfg);

    auto device = pipe->get_active_profile().get_device();
    rs2::playback playback = device.as<rs2::playback>();
    playback.set_real_time(false);

    auto duration = playback.get_duration();
    int progress = 0;
    auto frameNumber = 0ULL;
    
    rs2::frameset frameset;
    uint64_t posLast = playback.get_position();
    while (pipe->try_wait_for_frames(&frameset, 1000)) 
    {
        int posP = static_cast<int>(posLast * 100. / duration.count());

        if (posP > progress) {
            progress = posP;
            cout << posP << "%" << "\r" << flush;
        }



        frameNumber = frameset[0].get_frame_number();

        for_each(converters.begin(), converters.end(),
            [&frameset] (shared_ptr<rs2::tools::converter::converter_base>& converter) {
                converter->convert(frameset);
            });

        for_each(converters.begin(), converters.end(),
            [] (shared_ptr<rs2::tools::converter::converter_base>& converter) {
                converter->wait();
            });
        const uint64_t posCurr = playback.get_position();
        if(static_cast<int64_t>(posCurr - posLast) < 0){
            break;
        }
        posLast = posCurr;
    }

    cout << endl;

    for_each(converters.begin(), converters.end(),
        [] (shared_ptr<rs2::tools::converter::converter_base>& converter) {
            cout << converter->get_statistics() << endl;
        });

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
