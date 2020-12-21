// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../func-common.h"
#include "presets-common.h"
#include "l500/l500-options.h"

using namespace rs2;

TEST_CASE( "test-func-presets-calc-preset-change-controls", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_to_expected_map = build_preset_to_expected_values_map( depth_sens );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {

        REQUIRE( depth_sens.supports( RS2_OPTION_SENSOR_MODE ) );
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, rs2_sensor_mode( mode ) ) );

        set_values_manually(
            depth_sens,
            preset_to_expected_map[{ rs2_l500_visual_preset( preset ), rs2_sensor_mode( mode ) }] );

        CAPTURE( rs2_l500_visual_preset( preset ) );
        CAPTURE( rs2_sensor_mode( mode ) );

        validate_presets_value( depth_sens, rs2_l500_visual_preset( preset ) );
    } );

}
