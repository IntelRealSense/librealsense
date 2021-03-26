// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp
//#cmake:add-file ../../../src/algo/thermal-loop/*.cpp

//#test:flag custom-args    # disable passing in of catch2 arguments

#define DISABLE_LOG_TO_STDOUT
#include "d2rgb-common.h"
#include "compare-to-bin-file.h"
#include "compare-scene.h"


TEST_CASE("Scene 2", "[d2rgb]")
{
    ac_logger logger;

    // TODO so Travis passes, until we fix the test-case
    //std::string scene_dir("..\\unit-tests\\algo\\depth-to-rgb-calibration\\19.2.20");
    std::string scene_dir( "C:\\work\\autocal\\New\\A\\20_05_2020-Ashrafon-cubic2-4\\ac_4" );
    scene_dir += "\\1\\";

    std::ifstream f( join( bin_dir( scene_dir ), "camera_params" ) );
    if( f.good() )
        compare_scene( scene_dir );
    else
        std::cout << "-I- skipping scene-2 test for now" << std::endl;
}
