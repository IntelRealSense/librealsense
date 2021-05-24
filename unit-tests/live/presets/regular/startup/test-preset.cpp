// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "../../presets-common.h"
#include <src/l500/l500-options.h>

using namespace rs2;


TEST_CASE( "calc preset from controls on start up", "[l500][live]" )
{
    // This test will create for each preset value new rs2::device and destroy it
    // in order to simulate the scenario of:
    // 1. start up librealsense and create an object of L500 device 
    // 2. set specific preset to depth sensor
    // 3. close librealsense
    // 4. start up again librealsense
    // we expect that on the second start up librealsense will calculated preset correctly from the control values

    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    for( auto i = (int)RS2_L500_VISUAL_PRESET_NO_AMBIENT; i < (int)RS2_L500_VISUAL_PRESET_AUTOMATIC;
         i++ )
    {
        // this will create an object of L500 device set it preset and destroy it
        build_new_device_and_do( [&] (rs2::depth_sensor & depth_sens)
        {
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA ) );
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)i ) );
        } );

        // We set the preset on the previous device, and want to check that, for a new device, the
        // preset was calculated correctly from the control values
        build_new_device_and_do( [&] (rs2::depth_sensor & depth_sens)
        {
            CAPTURE( rs2_l500_visual_preset( i ) );
            check_preset_is_equal_to( depth_sens, rs2_l500_visual_preset( i ) );
        } );
    }
}
