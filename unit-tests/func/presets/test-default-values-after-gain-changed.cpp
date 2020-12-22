// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../func-common.h"
#include "presets-common.h"
#include "l500/l500-options.h"

using namespace rs2;

TEST_CASE( "check defaults after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    for( auto i = (float)RS2_DIGITAL_GAIN_HIGH; i <= (float)RS2_DIGITAL_GAIN_LOW; i++ )
    {
        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, i );
        auto expected_default_values = get_expected_default_values( dev );
        auto actual_default_values = get_actual_default_values( depth_sens );
        compare( actual_default_values, expected_default_values );
    }
}
