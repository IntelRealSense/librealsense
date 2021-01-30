// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../../../func-common.h"
#include "../../presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check currents after hw control changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_to_expected_map = build_preset_to_expected_values_map( depth_sens );

    reset_camera_preset_mode( depth_sens );

    auto sensor_mode_def
        = rs2_sensor_mode( (int)depth_sens.get_option_range( RS2_OPTION_SENSOR_MODE ).def );

    // print_presets_to_csv( depth_sens, preset_to_expected_map );
    for( int preset = RS2_L500_VISUAL_PRESET_NO_AMBIENT; preset < RS2_L500_VISUAL_PRESET_AUTOMATIC;
         preset++ )
    {
        for( auto& o : preset_dependent_options)
        {
            if( o == RS2_OPTION_DIGITAL_GAIN )
                continue;

            depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, preset );

            auto range = depth_sens.get_option_range( o );
            depth_sens.set_option( o, range.min );

            auto preset_to_expected
                = preset_to_expected_map[{ rs2_l500_visual_preset( preset ), sensor_mode_def }];

            preset_to_expected[o] = range.min;

            compare_expected_currents_to_actual( depth_sens, preset_to_expected);
        } 
    }
}
