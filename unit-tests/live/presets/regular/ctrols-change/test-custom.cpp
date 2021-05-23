// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "../../presets-common.h"
#include <src/l500/l500-options.h>

using namespace rs2;
TEST_CASE( "move to custom after hw control changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    reset_camera_preset_mode_to_defaults( depth_sens );

    for( auto o : preset_dependent_options )
    {
        if( o == RS2_OPTION_DIGITAL_GAIN )
            continue;

        auto range = depth_sens.get_option_range( o );
        depth_sens.set_option( o, range.min );

        rs2_l500_visual_preset curr_preset;
        REQUIRE_NOTHROW( curr_preset = ( rs2_l500_visual_preset )(int)depth_sens.get_option(
                             RS2_OPTION_VISUAL_PRESET ) );

        REQUIRE( curr_preset == RS2_L500_VISUAL_PRESET_CUSTOM );
    }
}
