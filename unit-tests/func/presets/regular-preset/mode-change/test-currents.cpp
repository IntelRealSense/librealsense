// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check currents after resolution changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_to_expected_map = build_preset_to_expected_values_map( depth_sens );

    // print_presets_to_csv( depth_sens, preset_to_expected_map );
    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode from_mode ) 
    {
        depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, from_mode );
        for( int to_mode = RS2_SENSOR_MODE_VGA; to_mode < RS2_SENSOR_MODE_COUNT; to_mode++ )
        {
            CAPTURE( preset, from_mode, to_mode );

            depth_sens.set_option( RS2_OPTION_SENSOR_MODE, to_mode );

            auto & preset_to_expected = preset_to_expected_map[{ rs2_l500_visual_preset( preset ),
                                                                 rs2_sensor_mode( to_mode ) }];

            compare_expected_currents_to_actual( depth_sens, preset_to_expected);
        } 
    } );
}
