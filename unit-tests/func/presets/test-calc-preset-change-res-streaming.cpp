// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../func-common.h"
#include "presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "calc preset after change resolution while streaming", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto expected_preset_values = build_preset_to_expected_values_map( depth_sens );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        auto depth = find_profile( depth_sens, RS2_STREAM_DEPTH, mode );
        auto ir = find_profile( depth_sens, RS2_STREAM_INFRARED, mode );
        auto confidence = find_profile( depth_sens, RS2_STREAM_CONFIDENCE, mode );

        do_while_streaming( depth_sens, { depth, ir, confidence }, [&]() {
            set_option_values( depth_sens, expected_preset_values[{ preset, mode }] );

            REQUIRE( depth_sens.supports( RS2_OPTION_SENSOR_MODE ) );
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, float( mode ) ) );

            CAPTURE( preset );
            CAPTURE( mode );

            check_preset_is_equal_to( depth_sens, preset );
        } );
    } );
}
