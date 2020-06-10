// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//Project properties to modify so tis test will work: (TODO - add to cmake)
// - C/C++ > General > Additional Include Directories:
//   add: C:\Users\rbettan\Documents\GitRepos\librealsense2\third-party\glfw\include
// - Linker > Input > Additional Dependencies:
// add: 
//     ..\..\..\..\third-party\glfw\src\Release\glfw3.lib
//     opengl32.lib
//     glu32.lib
// Add Reference glfw

//#cmake:add-file f450-common-display.h

#include "f450-common-display.h"


TEST_CASE( "START-STOP - streaming ", "[F450]" ) {
    using namespace std;
    
    rs2::context ctx;
    auto devices_list = ctx.query_devices();
    size_t device_count = devices_list.size();
    REQUIRE(device_count > 0);

    std::vector<rs2::device> devices;

    for (auto i = 0u; i < device_count; i++)
    {
        try
        {
            auto dev = devices_list[i];
            devices.emplace_back(dev);
        }
        catch (...)
        {
            std::cout << "Failed to created device. Check SDK logs for details" << std::endl;
        }
    }

    //------------------
    // Create a simple OpenGL window for rendering:
    window app(1280, 720, "RealSense Capture Example");

    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;
    // Declare rates printer for showing streaming rates of the enabled streams.
    rs2::rates_printer printer;
    
    rs2::config cfg;
    cfg.enable_stream(RS2_STREAM_INFRARED, 0, RS2_FORMAT_Y16);
    cfg.enable_stream(RS2_STREAM_INFRARED, 1, RS2_FORMAT_Y16);

    int iterations = 100;
    int frameCounter = 0;
    while (iterations--)
    {
        rs2::pipeline pipe;
        pipe.start(cfg);
        std::cout << "pipe started" << std::endl;
        //------------
        while (app && ++frameCounter < 2)
        {
            rs2::frameset data = pipe.wait_for_frames().apply_filter(printer).apply_filter(color_map);
            app.show(data);
        }
        frameCounter = 0;
        //------------
        pipe.stop();
        std::cout << "pipe stopped" << std::endl;
    }
    
    
}
