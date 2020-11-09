// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * While not streaming, activating Max Usable Range option should make sure the sensor mode is 'VGA'
//         if it is not VGA it will set the sensor mode to VGA
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Max Usable Range option set sensor mode if not streaming", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();

    mur.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_QVGA );
    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );
    CHECK_NOTHROW( mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1 ) );
    CHECK( RS2_SENSOR_MODE_VGA == mur.get_option( RS2_OPTION_SENSOR_MODE ) );
}
