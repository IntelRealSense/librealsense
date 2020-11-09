// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Enabling Max Usable Range option while streaming require Visual Preset = 'Max Range' ,
//         If not an exception should be thrown
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Max Usable Range option require Max Range Preset while streaming",
           "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();

    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_LOW_AMBIENT );
    rs2::stream_profile depth_vga_profile;
    auto profile_found = get_profile_by_stream_parameters( mur,
                                                           RS2_STREAM_DEPTH,
                                                           RS2_FORMAT_Z16,
                                                           640,
                                                           480,
                                                           depth_vga_profile );

    REQUIRE( profile_found );

    mur.open( depth_vga_profile );
    mur.start( []( rs2::frame f ) {} );
    // Verify failure when visual preset != Max Range
    CHECK_THROWS( mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1 ) );
    // Verify stay off
    CHECK_FALSE( mur.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ) );
}
