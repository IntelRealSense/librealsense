// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check currents after resolution changed by start stream", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto sens = dev.first< rs2::depth_sensor >();

    auto preset_to_expected_map = build_preset_to_expected_values_map( sens );
    auto preset_to_expected_defaults_map = build_preset_to_expected_defaults_map( dev, sens );

    reset_camera_preset( sens );

    // set preset before stream start
    check_presets_values_while_streaming(
        sens,
        preset_to_expected_map,
        preset_to_expected_defaults_map,
        [&]( rs2_sensor_mode mode, rs2_l500_visual_preset preset ) {
            REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
        } );

    // set preset after stream start
    check_presets_values_while_streaming(
        sens,
        preset_to_expected_map,
        preset_to_expected_defaults_map,
        []( rs2_sensor_mode mode, rs2_l500_visual_preset preset ) {},
        [&]( rs2_sensor_mode mode, rs2_l500_visual_preset preset ) {
            REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
        } );
}
