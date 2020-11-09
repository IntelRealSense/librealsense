// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies depth sensor max usable range option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * Main Success Scenario while not streaming, make sure all conditions for MUR option meet,
//         enable MUR and try to get MUR value while not streaming.
//       
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Not streaming - Enable option but fail read value", "[max-usable-range]" )
{
    auto depth_sensor
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_MAX_USABLE_RANGE );
    auto mur = depth_sensor.as< rs2::max_usable_range_sensor >();

    mur.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA );
    mur.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );
    mur.set_option( RS2_OPTION_ENABLE_MAX_USABLE_RANGE, 1 );

    float mur_val = 0.0f;
    CHECK_THROWS(
        mur_val = mur.get_max_usable_depth_range() );  // Verify function throws when not streaming
    CHECK_FALSE( mur_val );
}
