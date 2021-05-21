// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "presets-common.h"
#include "../reset-camera.h"
#include <src/l500/l500-options.h>

using namespace rs2;

TEST_CASE( "calc preset after hw reset", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto new_dev = reset_camera_and_wait_for_connection( dev );
    auto depth_sens = new_dev.first< rs2::depth_sensor >();

    // Camera should boot up with max-range preset, always
    CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ) == RS2_L500_VISUAL_PRESET_MAX_RANGE );
}
