// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../func-common.h"
#include "presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "move to custom after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

        REQUIRE_NOTHROW(
            depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_NO_AMBIENT ) );

        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, RS2_DIGITAL_GAIN_LOW ) );

        rs2_l500_visual_preset curr_preset;
        REQUIRE_NOTHROW( curr_preset = ( rs2_l500_visual_preset )(int)depth_sens.get_option(
                             RS2_OPTION_VISUAL_PRESET ) );

        REQUIRE( curr_preset == RS2_L500_VISUAL_PRESET_CUSTOM );
}
