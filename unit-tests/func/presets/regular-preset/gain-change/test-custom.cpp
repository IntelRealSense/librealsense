// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;
TEST_CASE( "move to custom after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    // print_presets_to_csv( depth_sens, preset_to_expected_map );
    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) 
    {
        for( int gain = RS2_DIGITAL_GAIN_HIGH; gain < RS2_DIGITAL_GAIN_LOW; gain++ )
        {
            depth_sens.set_option( RS2_OPTION_SENSOR_MODE, mode );
            depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, preset );
            depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, gain );

            rs2_l500_visual_preset curr_preset;
            REQUIRE_NOTHROW( curr_preset = ( rs2_l500_visual_preset )(int)depth_sens.get_option(
                                 RS2_OPTION_VISUAL_PRESET ) );

            REQUIRE( curr_preset == RS2_L500_VISUAL_PRESET_CUSTOM );
        }
    } );
}