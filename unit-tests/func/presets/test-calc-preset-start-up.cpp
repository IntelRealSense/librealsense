// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../func-common.h"
#include "presets-common.h"
#include "l500/l500-options.h"

using namespace rs2;

TEST_CASE( "calc preset from controls on start up", "[l500][live]" )
{
    for( auto i = (int)RS2_L500_VISUAL_PRESET_NO_AMBIENT; i < (int)RS2_L500_VISUAL_PRESET_COUNT;
         i++ )
    {
        build_new_device_an_do( [&] (rs2::depth_sensor & depth_sens)
        {
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, RS2_SENSOR_MODE_VGA ) );
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, i ) );
        } );

        build_new_device_an_do( [&] (rs2::depth_sensor & depth_sens)
        {
            CAPTURE( rs2_l500_visual_preset( i ) );
            
            validate_presets_value( depth_sens, rs2_l500_visual_preset( i ) );
        } );
    }
}
