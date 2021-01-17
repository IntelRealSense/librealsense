// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../func-common.h"
#include "../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check defaults after preset changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto sens = dev.first< rs2::depth_sensor >();

    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for_each_preset_mode_combination(
        [&]( rs2_l500_visual_preset from_preset, rs2_sensor_mode from_mode ) {
            REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_SENSOR_MODE, (float)from_mode ) );
            REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)from_preset ) );
            CHECK( sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)from_preset );

            for_each_preset_mode_combination(
                [&]( rs2_l500_visual_preset to_preset, rs2_sensor_mode to_mode ) {
                    sens.set_option( RS2_OPTION_SENSOR_MODE, to_mode );
                    auto expected_default_values = get_defaults_from_fw( dev );
                    auto actual_default_values = get_defaults_from_lrs( sens );
                    compare( actual_default_values, expected_default_values );
                } );
        } );
}
