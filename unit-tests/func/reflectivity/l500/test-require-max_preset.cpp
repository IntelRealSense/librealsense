// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies IR reflectivity option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Enabling IR Reflectivity option while streaming require Visual Preset = 'Max Range'
//         If not an exception should be thrown
//
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Reflectivity while streaming require Max Range Preset", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    // Make sure alt IR is off
    if( ds.supports( RS2_OPTION_ALTERNATE_IR ) )
        ds.set_option( RS2_OPTION_ALTERNATE_IR, 0 );

    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_LOW_AMBIENT );
    rs2::stream_profile depth_vga_profile;
    auto profile_found = get_profile_by_stream_parameters( ds,
                                                           RS2_STREAM_DEPTH,
                                                           RS2_FORMAT_Z16,
                                                           640,
                                                           480,
                                                           depth_vga_profile );
    REQUIRE( profile_found );

    ds.open( depth_vga_profile );
    ds.start( []( rs2::frame f ) {} );

    // Verify failure
    CHECK_THROWS( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1 ) );
    // Verify stay off
    CHECK_FALSE( ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) );
}
