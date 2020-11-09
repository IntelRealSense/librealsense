// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies IR reflectivity option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Streaming on test
//         Activating IR Reflectivity while Max Usable Range is off
//         should turn on Max Usable Range option.
//         If the MUR option was turned on by activating IR Reflectivity,
//         Turning off IR Reflectivity should turn off MUR option.
//
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Not streaming - automatically turn on/off MUR option", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    // Verify alt IR is off
    if( ds.supports( RS2_OPTION_ALTERNATE_IR ) )
        ds.set_option( RS2_OPTION_ALTERNATE_IR, 0 );

    ds.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA );
    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );
    CHECK_NOTHROW( ds.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0 ) );

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

    // Verify automatically turn on of MUR option
    CHECK_NOTHROW( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1 ) );
    CHECK( ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) );
    CHECK( ds.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ) );

    // Verify automatically turn off of MUR option
    CHECK_NOTHROW( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 0 ) );
    CHECK_FALSE( ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) );
    CHECK_FALSE( ds.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ) );
}
