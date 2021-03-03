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
//       * While not streaming, activating IR Reflectivity option should make sure the visual preset is 'Max Range' and the sensor mode is 'VGA', 
//         if it is not it will set the visual preset to max range and the sensor mode to 'VGA'.
//       * Starting non VGA stream when IR Reflectivity is on, turns it off.
//       * Activating IR Reflectivity while Max Usable Range is off
//         should turn on Max Usable Range option.
//         If the MUR option was turned on by activating IR Reflectivity,
//         Turning off IR Reflectivity should turn off MUR option.
//       * If MUR option was already on when IR Reflectivity was turned on,
//         turning off IR Reflectivity should not turn off MUR option.
//       * While streaming, Activating IR Reflectivity while Max Usable Range is off
//         should turn on Max Usable Range option.
//         If the MUR option was turned on by activating IR Reflectivity,
//         Turning off IR Reflectivity should turn off MUR option.
//       * Verify turning off MUR option turns off IR Reflectivity option,
//         Reflectivity require MUR to be on otherwise it can't work.
//
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected.
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "IR Reflectivity option change visual preset and sensor mode if not streaming", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    // Verify alt IR is off
    if( ds.supports( RS2_OPTION_ALTERNATE_IR ) )
        ds.set_option( RS2_OPTION_ALTERNATE_IR, 0 );

    ds.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_XGA );
    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_SHORT_RANGE );
    CHECK_NOTHROW( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f ) );
    CHECK( RS2_L500_VISUAL_PRESET_MAX_RANGE == ds.get_option( RS2_OPTION_VISUAL_PRESET ) );
    CHECK(RS2_SENSOR_MODE_VGA == ds.get_option(RS2_OPTION_SENSOR_MODE));
}

TEST_CASE("Starting streaming non VGA sensor mode turns off IR Reflectivity", "[reflectivity]")
{
    auto ds
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY);

    // Verify alt IR is off
    if (ds.supports(RS2_OPTION_ALTERNATE_IR))
        ds.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    ds.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);
    ds.set_option(RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA);

    // Verify activating IR Reflectivity
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));


    auto profiles = ds.get_stream_profiles();
    // Find a profile that sensor mode != VGA
    auto non_vga_depth_profile
        = std::find_if(profiles.begin(), profiles.end(), [](rs2::stream_profile sp) {
        auto vp = sp.as< rs2::video_stream_profile >();

        return ((sp.stream_type() == RS2_STREAM_DEPTH) && (sp.format() == RS2_FORMAT_Z16)
            && (vp.width() != 640) && (vp.height() != 480));
            });

    REQUIRE(non_vga_depth_profile != profiles.end());
    ds.open(*non_vga_depth_profile);
    ds.start([](rs2::frame f) {});

    // Verify turned off
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));

    ds.stop();
    ds.close();
}

TEST_CASE("Automatically turn on MUR option - non streaming", "[reflectivity]")
{
    auto ds
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY);

    // Verify alt IR is off
    if (ds.supports(RS2_OPTION_ALTERNATE_IR))
        ds.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    ds.set_option(RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA);
    ds.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0));
    // Verify automatically turn on of MUR option
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    // Verify automatically turn off of MUR option
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 0));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    // Turn on MUR
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1));

    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 0));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    // Verify MUR stays on
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));
}


TEST_CASE("Automatically turn on/off MUR option - streaming", "[reflectivity]")
{
    auto ds
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY);

    // Verify alt IR is off
    if (ds.supports(RS2_OPTION_ALTERNATE_IR))
        ds.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    ds.set_option(RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA);
    ds.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0));

    auto depth_vga_profile
        = get_profile_by_stream_parameters( ds, RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 30 );

    REQUIRE(depth_vga_profile);

    ds.open(depth_vga_profile);
    ds.start([](rs2::frame f) {});

    // Verify automatically turn on of MUR option
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    // Verify automatically turn off of MUR option
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 0));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    // Turn on MUR
    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1));

    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    CHECK_NOTHROW(ds.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 0));
    CHECK_FALSE(ds.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    // Verify MUR stays on
    CHECK(ds.get_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE));

    ds.stop();
    ds.close();
}

TEST_CASE("Turning off MUR option turns off IR Reflectivity", "[reflectivity]")
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit("L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE);
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();
    REQUIRE(mur);
    mur.set_option(RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE);

    // Verify alt IR is off before enabling IR Reflectivity
    if (mur.supports(RS2_OPTION_ALTERNATE_IR))
        mur.set_option(RS2_OPTION_ALTERNATE_IR, 0);

    auto depth_vga_profile
        = get_profile_by_stream_parameters( mur, RS2_STREAM_DEPTH, RS2_FORMAT_Z16, 640, 480, 30 );

    REQUIRE(depth_vga_profile);

    // Check while not streaming
    mur.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1.f);
    mur.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f);
    CHECK(mur.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    mur.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0);
    CHECK_FALSE(mur.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));

    mur.open(depth_vga_profile);
    mur.start([](rs2::frame f) {});

    // Check while streaming
    mur.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1.f);
    mur.set_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1.f);
    CHECK(mur.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));
    mur.set_option(RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 0);
    CHECK_FALSE(mur.get_option(RS2_OPTION_ENABLE_IR_REFLECTIVITY));

    mur.stop();
    mur.close();
}
