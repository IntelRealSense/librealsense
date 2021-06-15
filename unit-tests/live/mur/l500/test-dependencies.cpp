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
//       * While not streaming, activating Max Usable Range option should make sure the visual preset is 'Max Range'
//         and sensor mode is 'VGA'
//         if it is not it will set the visual preset to max range and the sensor mode to 'VGA'
//       * Reading Max Usable Range while Max Usable Range option is off or not streaming - expect exception thrown
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Max Usable Range option change visual preset and sensor mode if not streaming", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();
    REQUIRE(mur);
    mur.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_XGA );
    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_SHORT_RANGE );
    CHECK_NOTHROW( mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1.f ) );
    CHECK( RS2_L500_VISUAL_PRESET_MAX_RANGE == mur.get_option( RS2_OPTION_VISUAL_PRESET ) );
    CHECK( RS2_SENSOR_MODE_VGA == mur.get_option( RS2_OPTION_SENSOR_MODE ) );

}

