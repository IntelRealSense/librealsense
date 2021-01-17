// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../func-common.h"
#include "../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check currents after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode ) );
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)preset );

        for( auto i = (float)RS2_DIGITAL_GAIN_HIGH; i <= (float)RS2_DIGITAL_GAIN_LOW; i++ )
        {
            depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, i );
            auto expected_default_values = get_defaults_from_fw( dev );
            auto actual_currents_values = get_currents_from_lrs( depth_sens );
            compare( actual_currents_values, expected_default_values );
        }
    } );
}
