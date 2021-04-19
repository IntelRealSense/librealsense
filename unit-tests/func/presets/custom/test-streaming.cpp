// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "../../func-common.h"
#include "../presets-common.h"

using namespace rs2;

// This test checks that if we are on custom preset and start streaming,
// with profile that has different resolution than the value of the 
// sensor mode control LRS don't throw an exception and update the
// sensor mode according the profile
TEST_CASE( "sensor mode change while streaming on custom", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    reset_camera_preset_mode_to_defaults( depth_sens );

    REQUIRE_NOTHROW(
        depth_sens.set_option(RS2_OPTION_VISUAL_PRESET,RS2_L500_VISUAL_PRESET_CUSTOM));

    start_depth_ir_confidence(depth_sens, RS2_SENSOR_MODE_XGA);
    
    auto curr_sensor_mode = RS2_SENSOR_MODE_VGA;
    REQUIRE_NOTHROW(curr_sensor_mode = (rs2_sensor_mode)(int)depth_sens.get_option(RS2_OPTION_SENSOR_MODE));
    REQUIRE(curr_sensor_mode == RS2_SENSOR_MODE_XGA);

    stop_depth(depth_sens);
}
