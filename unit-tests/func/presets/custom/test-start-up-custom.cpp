// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../func-common.h"
#include "../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "calc preset from controls on start up whan values are not fit none of the visual preset", "[l500][live]" )
{
    // This test will create for each preset value new rs2::device and destroy it
    // in order to simulate the scenario of:
    // 1. start up librealsense and create an object of L500 device
    // 2. set specific preset to depth sensor
    // 3. close librealsense
    // 4. start up again librealsense
    // we expect that on the second start up librealsense will calculated preset correctly from the
    // control values

    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];
    auto depth_sens = dev.first< rs2::depth_sensor >();

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    // low gain with values of preset no ambient should not fit none of the visual preset
    REQUIRE_NOTHROW(
        depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_NO_AMBIENT ) );

    REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, RS2_DIGITAL_GAIN_LOW ) );

    rs2_l500_visual_preset curr_preset;
    REQUIRE_NOTHROW( curr_preset = ( rs2_l500_visual_preset )(int)depth_sens.get_option(
                         RS2_OPTION_VISUAL_PRESET ) );

    REQUIRE( curr_preset == RS2_L500_VISUAL_PRESET_CUSTOM );

    // check that, for a new device, the preset is custom
    build_new_device_and_do( [&]( rs2::depth_sensor & depth_sens ) {
        check_preset_is_equal_to( depth_sens, RS2_L500_VISUAL_PRESET_CUSTOM );
    } );
}
