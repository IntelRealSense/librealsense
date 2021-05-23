// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!
//#test:device L500*

#include "presets-common.h"

using namespace rs2;

TEST_CASE( "presets sanity", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

    preset_values_map expected_values, expected_defs;
    build_presets_to_expected_values_defs_map(dev, depth_sens, expected_values, expected_defs);
   
     //print_presets_to_csv( depth_sens, preset_to_expected_defaults_map);

    check_presets_values(
        { RS2_L500_VISUAL_PRESET_NO_AMBIENT,
          RS2_L500_VISUAL_PRESET_LOW_AMBIENT,
          RS2_L500_VISUAL_PRESET_MAX_RANGE,
          RS2_L500_VISUAL_PRESET_SHORT_RANGE },
        depth_sens,
        expected_values,
        expected_defs,
        [&]( preset_mode_pair preset_mode ) { set_mode_preset( depth_sens, preset_mode ); } );
}
