// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#test:tag reflectivity
//#test:device L500*

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies IR reflectivity option restrictions,
//         according to [RS-8358].
//
// Current test description:
//       * Enabling IR Reflectivity option while streaming require Visual Preset = 'Max Range' and Sensor Mode = 'VGA'.
//         If not an exception should be thrown.
//       * If Alternate IR option is on, turning on IR Reflectivity 
//         should throw an exception.
//
// Note: * L515 specific test.
//       * Test will pass if no L515 device is connected.
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Reflectivity while streaming require Max Range Preset", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    // Make sure alt IR is off
    if( ds.supports( RS2_OPTION_ALTERNATE_IR ) )
        ds.set_option( RS2_OPTION_ALTERNATE_IR, 0 );

    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_LOW_AMBIENT );
    auto depth_vga_profile
        = get_profile_by_stream_parameters( ds, RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 30 );
    REQUIRE( depth_vga_profile );

    ds.open( depth_vga_profile );
    ds.start( []( rs2::frame f ) {} );

    // Verify failure
    CHECK_THROWS( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f ) );
    // Verify stay off
    CHECK_FALSE( ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) );

    ds.stop();
    ds.close();
}


TEST_CASE("Reflectivity while streaming require VGA Sensor Mode", "[reflectivity]")
{
    auto ds
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY);

    // Make sure alt IR is off
    if (ds.supports(RS2_OPTION_ALTERNATE_IR))
        ds.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    ds.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);

    // Find a profile that sensor mode != VGA
    auto profiles = ds.get_stream_profiles();
    auto non_vga_depth_profile
        = std::find_if(profiles.begin(), profiles.end(), [](rs2::stream_profile sp) {
        auto vp = sp.as< rs2::video_stream_profile >();

        return ((sp.stream_type() == RS2_STREAM_DEPTH) && (sp.format() == RS2_FORMAT_Z16)
            && (vp.width() != 640) && (vp.height() != 480));
            });
    
    REQUIRE(non_vga_depth_profile != profiles.end());

    ds.open(*non_vga_depth_profile);
    ds.start([](rs2::frame f) {});

    // Verify failure
    CHECK_THROWS(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    // Verify stay off
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));

    ds.stop();
    ds.close();
}

TEST_CASE("Enable IR Reflectivity when Alternate IR is on fails", "[reflectivity]")
{
    auto ds
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY);

    if (!ds.supports(RS2_OPTION_ALTERNATE_IR))
    {
        std::cout << "Sensor not supporting alternate IR, test skipped" << std::endl;
        exit(0);
    }

    ds.set_option(RS2_OPTION_ALTERNATE_IR, 1.f);

    ds.set_option(RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA);
    ds.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);

    CHECK_THROWS(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
}
