// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../func-common.h"
#include "presets-common.h"

using namespace rs2;

TEST_CASE( "presets sanity", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    auto preset_to_expected_map = build_preset_to_expected_values_map( depth_sens );

    print_presets_to_csv( depth_sens, preset_to_expected_map );

    check_presets_values( depth_sens, preset_to_expected_map );

}
