// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Reading Max Usable Range while Max Usable Range option is off - expect exception thrown
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Reading Max Usable Range while option is off - Fail Test", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();

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

    // Verify MUR option is off
    mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0 );

    float mur_val = 0.0f;
    // Verify function throws when MUR option is off
    CHECK_THROWS( mur_val = mur.get_max_usable_depth_range() );
    CHECK_FALSE( mur_val );
}
