// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#test:tag max-usable-range
//#test:device L500*

#include "../../live-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * All conditions for Max Usable Range set, try to get MUR value while not streaming (should throw).
//       * All conditions for Max Usable Range set and depth sensor is streaming, verify calling success.
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Streaming - Sanity", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();
    REQUIRE(mur);
    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );

    auto depth_vga_profile
        = get_profile_by_stream_parameters( mur, RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 30 );

    REQUIRE( depth_vga_profile );

    mur.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1.f);
    
    float mur_val = 0.0f;
    CHECK_THROWS(mur.get_max_usable_depth_range());  // Verify function throws when not streaming

    mur.open( depth_vga_profile );
    mur.start( []( rs2::frame f ) {} );


    CHECK_NOTHROW( mur_val = mur.get_max_usable_depth_range() );
    CHECK( 0 != mur_val );

    mur.stop();
    mur.close();
}
