// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include "../func-common.h"
#include "presets-common.h"
#include <l500/l500-options.h>

using namespace rs2;

TEST_CASE( "check defaults after gain changed", "[l500][live]" )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    exit_if_fw_version_is_under( dev, MIN_GET_DEFAULT_FW_VERSION );

    auto depth_sens = dev.first< rs2::depth_sensor >();

     for( auto gain = (float)RS2_DIGITAL_GAIN_HIGH; gain <= (float)RS2_DIGITAL_GAIN_LOW; gain++ )
    {
         depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, gain );
     
        for( auto mode = (float)RS2_SENSOR_MODE_VGA; mode < (float)RS2_SENSOR_MODE_COUNT; mode++ )
        {
            depth_sens.set_option( RS2_OPTION_SENSOR_MODE, mode );
            auto expected_default_values = get_defaults_from_fw( dev );
            auto actual_default_values = get_defaults_from_lrs( depth_sens );
            compare( actual_default_values, expected_default_values );
        }
    }
}
