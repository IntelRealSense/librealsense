// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L515

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

// This test checks that if we move to auto preset while streaming, current and default values 
// of all the controls are correct
TEST_CASE( "move from regular preset to automatic ", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    REQUIRE_NOTHROW(
        depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, RS2_L500_VISUAL_PRESET_AUTOMATIC ) );
    REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA ) );

    start_depth_ir_confidence( depth_sens, RS2_SENSOR_MODE_VGA );

    std::map< rs2_option, float > expected_values, expected_defs;
    build_preset_to_expected_values_map( dev,
                                         depth_sens,
                                         { RS2_L500_VISUAL_PRESET_AUTOMATIC, RS2_SENSOR_MODE_VGA },
                                         expected_values,
                                         expected_defs );

    compare_to_actual( depth_sens, expected_values, expected_defs );

    stop_depth( depth_sens );
}
