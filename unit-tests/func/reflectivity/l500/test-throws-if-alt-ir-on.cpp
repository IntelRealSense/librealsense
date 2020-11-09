// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../func-common.h"

// Test group description:
//       * This tests group verifies IR reflectivity option restrictions,
//         according to [RS-8358]
//
// Current test description:
//       * If Alternate IR option is on, turning on IR Reflectivity 
//         should throw an exception
//
// Note: * L515 specific test
//       * Test will pass if no L515 device is connected
//       * Test will be performed on the first L515 device detected.

TEST_CASE( "Enable IR Reflectivity when Alternate IR is on fails", "[reflectivity]" )
{
    auto ds
        = find_first_supported_depth_sensor_or_exit( "L515", RS2_OPTION_ENABLE_IR_REFLECTIVITY );

    if( ! ds.supports( RS2_OPTION_ALTERNATE_IR ) )
    {
        std::cout << "Sensor not supporting alternate IR, test skipped" << std::endl;
        exit( 0 );
    }

    ds.set_option( RS2_OPTION_ALTERNATE_IR, 1 );

    ds.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA );
    ds.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_MAX_RANGE );

    CHECK_THROWS( ds.set_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY, 1 ) );
    CHECK_FALSE( ds.get_option( RS2_OPTION_ENABLE_IR_REFLECTIVITY ) );
}
