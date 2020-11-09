// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies IR reflectivity option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * While not streaming, activating IR Reflectivity option should make sure the visual preset is 'Max Range', 
//         if it is not it will set the visual preset to max range
//
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "IR Reflectivity option change visual preset if not streaming", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    // Verify alt IR is off
    if( ds.supports( RS2_OPTION_ALTERNATE_IR ) )
        ds.set_option( RS2_OPTION_ALTERNATE_IR, 0 );

    ds.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA );
    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_SHORT_RANGE );
    CHECK_NOTHROW( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1 ) );
    CHECK( RS2_L500_VISUAL_PRESET_MAX_RANGE == ds.get_option( RS2_OPTION_VISUAL_PRESET ) );
}
