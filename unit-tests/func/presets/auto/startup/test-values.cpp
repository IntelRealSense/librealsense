// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L515

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check current values on start up", "[l500][live]" )
{
    // This test will create new rs2::device and destroy it
    // in order to simulate the scenario of:
    // 1. start up librealsense and create an object of L500 device
    // 2. set specific preset to depth sensor
    // 3. close librealsense
    // 4. start up again librealsense
    // we expect that control values will not change

    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];
    auto sens = dev.first< rs2::depth_sensor >();

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    std::map< rs2_option, float > expected_values, expected_defs;
    build_preset_to_expected_values_map( dev,
                                         sens,
                                         { RS2_L500_VISUAL_PRESET_AUTOMATIC, RS2_SENSOR_MODE_VGA },
                                         expected_values,
                                         expected_defs );


    // this will create an object of L500 device set it preset and destroy it
    build_new_device_and_do( [&]( rs2::depth_sensor & sens ) {
        REQUIRE_NOTHROW(
            sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)RS2_L500_VISUAL_PRESET_AUTOMATIC ) );
    } );

    //// We set the preset on the previous device, and want to check that, for a new device, the
    //// preset was calculated correctly from the control values
    build_new_device_and_do( [&]( rs2::depth_sensor & sens ) {
        CAPTURE( RS2_L500_VISUAL_PRESET_AUTOMATIC );

        auto sensor_mode_def
            = rs2_sensor_mode( (int)sens.get_option_range( RS2_OPTION_SENSOR_MODE ).def );

        compare_to_actual( sens, expected_values, expected_defs );
    } );
}