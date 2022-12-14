// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*
//#test:donotrun      # temporary: currently it fails LibCI on Linux (see LRS-351)!

#include "presets-common.h"

using namespace rs2;

TEST_CASE( "presets sanity while streaming", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    preset_values_map expected_values, expected_defs;
    build_presets_to_expected_values_defs_map(dev, depth_sens, expected_values, expected_defs);
   
    reset_camera_preset_mode_to_defaults( depth_sens );

    // set preset and mode before stream start
    check_presets_values_while_streaming(
        { RS2_L500_VISUAL_PRESET_NO_AMBIENT, RS2_L500_VISUAL_PRESET_LOW_AMBIENT },
        depth_sens,
        expected_values,
        expected_defs,
        [&]( preset_mode_pair preset_mode ) {
            set_mode_preset( depth_sens, preset_mode );
        }
    );

    // set preset and mode after stream start
    check_presets_values_while_streaming(
        { RS2_L500_VISUAL_PRESET_MAX_RANGE, RS2_L500_VISUAL_PRESET_SHORT_RANGE },
        depth_sens,
        expected_values,
        expected_defs,
        []( preset_mode_pair preset_mode ) {},
        [&]( preset_mode_pair preset_mode ) {
            // We don't want to change the sensor mode while streaming (it's read-only)!
            //set_mode_preset( depth_sens, preset_mode );
            REQUIRE_NOTHROW(depth_sens.set_option(RS2_OPTION_VISUAL_PRESET, (float)preset_mode.first));
            CHECK(depth_sens.get_option(RS2_OPTION_VISUAL_PRESET) == (float)preset_mode.first);
        }
    );
}
