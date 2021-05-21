// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "../../presets-common.h"
#include <src/l500/l500-options.h>

using namespace rs2;

// This test checks that after changing resolution, current and default values 
// of all the others controls are correct
TEST_CASE( "check currents after resolution changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    preset_values_map expected_values, expected_defs;
    build_presets_to_expected_values_defs_map(dev, depth_sens, expected_values, expected_defs);

    // print_presets_to_csv( depth_sens, preset_to_expected_map );
    for_each_preset_mode_combination( [&]( preset_mode_pair preset_mode) {
        depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset_mode.first );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)preset_mode.second );

        CAPTURE(preset_mode);

        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)preset_mode.second);

        compare_to_actual(depth_sens, expected_values[preset_mode], expected_defs[preset_mode]);
    } );
}
