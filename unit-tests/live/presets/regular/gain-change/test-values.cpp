// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "../../presets-common.h"
#include <src/l500/l500-options.h>

using namespace rs2;

// This test checks that after changing digital gain, current and default values 
// of all the others controls are correct
TEST_CASE( "check currents and defaults values after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_mode_def = get_camera_preset_mode_defaults( depth_sens );

    std::map< rs2_option, float > expected_values, expected_defs;

    build_preset_to_expected_values_map( dev, depth_sens, preset_mode_def, expected_values, expected_defs);

    for( int gain = RS2_DIGITAL_GAIN_HIGH; gain < RS2_DIGITAL_GAIN_LOW; gain++ )
    {
        reset_camera_preset_mode_to_defaults( depth_sens );

        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, (float)gain );

        expected_values[RS2_OPTION_DIGITAL_GAIN] = (float)gain;

        compare_to_actual( depth_sens, expected_values, expected_defs);
    }
}
