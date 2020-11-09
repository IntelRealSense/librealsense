// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <thread>
#include <chrono>
#include "../../func-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Enabling Max Usable Range option while streaming require Sensor Mode = 'VGA' 
//         If not an exception should be thrown
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Max Usable Range option require VGA sensor mode while streaming", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();

    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );
    auto profiles = mur.get_stream_profiles();
    // Find a profile that sensor mode != VGA
    auto non_vga_depth_profile
        = std::find_if( profiles.begin(), profiles.end(), []( rs2::stream_profile sp ) {
              auto vp = sp.as< rs2::video_stream_profile >();

              return ( ( sp.stream_type() == RS2_STREAM_DEPTH ) && ( sp.format() == RS2_FORMAT_Z16 )
                       && ( vp.width() != 640 ) && ( vp.height() != 480 ) );
          } );

    REQUIRE( non_vga_depth_profile != profiles.end() );
    mur.open( *non_vga_depth_profile );

    mur.start( []( rs2::frame f ) {} );
    // Verify failure when sensor mode != VGA
    CHECK_THROWS( mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1 ) );
    // Verify stay off
    CHECK_FALSE( mur.get_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE ) );
}
