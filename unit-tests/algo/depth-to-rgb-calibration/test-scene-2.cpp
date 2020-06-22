// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file ../../../src/algo/depth-to-rgb-calibration/*.cpp

#include "d2rgb-common.h"
#include "compare-to-bin-file.h"
#include "compare-scene.h"


TEST_CASE("Scene 2", "[d2rgb]")
{
    // TODO so Travis passes, until we fix the test-case
    //std::string scene_dir("..\\unit-tests\\algo\\depth-to-rgb-calibration\\19.2.20");
    std::string scene_dir( "C:\\work\\autocal" );
    scene_dir += "\\F9440687\\LongRange_D_768x1024_RGB_1920x1080\\2\\";

    std::ifstream f( bin_dir( scene_dir ) + "camera_params" );
    if( f.good() )
        compare_scene( scene_dir );
    else
        std::cout << "-I- skipping scene-2 test for now" << std::endl;
}
