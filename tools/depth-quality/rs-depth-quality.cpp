// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp>
#include "depth-quality-model.h"
#include "ux-window.h"
#include <iostream>

#ifdef WIN32
#include <Windows.h> // On Windows we want the tool to behave as a proper Windows application
                     // rather then a console App
#endif

int main(int argc, const char * argv[]) try
{
    rs2::depth_quality::tool_model model;
    rs2::ux_window window()

    rs2::pipeline pipe;

    pipe.enable_stream(RS2_STREAM_DEPTH, 0, 0, 0, RS2_FORMAT_Z16, 30);
    pipe.enable_stream(RS2_STREAM_INFRARED, 1, 0, 0, RS2_FORMAT_Y8, 30);

    // Wait till a valid device is found
    pipe.start();

    // Select the depth camera to work with
    app.update_configuration(pipe);

    while (app)     // Update internal state and render UI
    {
        rs2::frameset frames;
        if (pipe.poll_for_frames(&frames))
        {
            app.upload(frames);

            rs2::frame depth = frames.get_depth_frame();
            if (depth)
                app.enqueue_for_processing(depth);
        }
    }

    pipe.stop();
    return EXIT_SUCCESS;
}
catch (const rs2::error& e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception& e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
#ifdef WIN32
int CALLBACK WinMain(
    _In_ HINSTANCE hInstance,
    _In_ HINSTANCE hPrevInstance,
    _In_ LPSTR     lpCmdLine,
    _In_ int       nCmdShow

)
{
    int argCount;

    std::shared_ptr<LPWSTR> szArgList(CommandLineToArgvW(GetCommandLine(), &argCount), LocalFree);
    if (szArgList == NULL) return main(0, nullptr);

    std::vector<std::string> args;
    for (int i = 0; i < argCount; i++)
    {
        std::wstring ws = szArgList.get()[i];
        std::string s(ws.begin(), ws.end());
        args.push_back(s);
    }

    std::vector<const char*> argc;
    std::transform(args.begin(), args.end(), std::back_inserter(argc), [](const std::string& s) { return s.c_str(); });

    return main(argc.size(), argc.data());
}
#endif

