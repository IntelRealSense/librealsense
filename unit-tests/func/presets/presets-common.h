// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <concurrency.h>

using namespace rs2;

#define preset_to_expected_values_map                                                              \
    std ::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >, std::map< rs2_option, float > >

void for_each_preset_mode_combination(
    std::function< void( rs2_l500_visual_preset, rs2_sensor_mode ) > action )
{
    for( auto i = (int)RS2_L500_VISUAL_PRESET_NO_AMBIENT; i < (int)RS2_L500_VISUAL_PRESET_COUNT;
         i++ )
    {
        for( auto j = (int)RS2_SENSOR_MODE_VGA; j < (int)RS2_SENSOR_MODE_COUNT; j++ )
        {
            action( rs2_l500_visual_preset( i ), rs2_sensor_mode( j ) );
        }
    }
}

preset_to_expected_values_map build_preset_to_expected_values_map( rs2::depth_sensor & depth_sens )
{
    std::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >, std::map< rs2_option, float > >
        preset_to_expected_values;

    std::map< rs2_l500_visual_preset, rs2_digital_gain > preset_to_gain
        = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, RS2_DIGITAL_GAIN_HIGH},
            { RS2_L500_VISUAL_PRESET_MAX_RANGE, RS2_DIGITAL_GAIN_HIGH },
            { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, RS2_DIGITAL_GAIN_LOW },
            { RS2_L500_VISUAL_PRESET_SHORT_RANGE, RS2_DIGITAL_GAIN_LOW } };


    std::vector< rs2_option > preset_dependent_options = { RS2_OPTION_POST_PROCESSING_SHARPENING,
                                                             RS2_OPTION_PRE_PROCESSING_SHARPENING,
                                                             RS2_OPTION_NOISE_FILTERING,
                                                             RS2_OPTION_CONFIDENCE_THRESHOLD,
                                                             RS2_OPTION_INVALIDATION_BYPASS,
                                                             RS2_OPTION_DIGITAL_GAIN,
                                                             RS2_OPTION_LASER_POWER,
                                                             RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                                                             RS2_OPTION_MIN_DISTANCE };

    
    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) 
    { 
        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, preset_to_gain[preset] );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, mode );

         auto laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER );
        std::map< rs2_l500_visual_preset, float > preset_to_laser
            = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, laser_range.def },
                { RS2_L500_VISUAL_PRESET_MAX_RANGE, laser_range.max },
                { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, laser_range.max },
                { RS2_L500_VISUAL_PRESET_SHORT_RANGE, laser_range.def } };

        for (auto dependent_option : preset_dependent_options)
        {
            preset_to_expected_values[{ preset, mode }][dependent_option]
                = depth_sens.get_option_range( dependent_option ).def;
        }
        preset_to_expected_values[{ preset, mode }][RS2_OPTION_DIGITAL_GAIN]
            = preset_to_gain[preset];

        preset_to_expected_values[{ preset, mode }][RS2_OPTION_LASER_POWER]
            = preset_to_laser[preset];
    } );

    return preset_to_expected_values;
}

void print_presets_to_csv( rs2::depth_sensor & depth_sens,
                           preset_to_expected_values_map & preset_to_expected_values )
{
    REQUIRE_NOTHROW( depth_sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    std::ofstream csv( "presets.csv" );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) 
    {
        auto expected_values
            = preset_to_expected_values[{ rs2_l500_visual_preset( preset ), mode }];

        csv << "-----" << rs2_l500_visual_preset( preset ) << " " << rs2_sensor_mode( mode )
            << "-----"
            << "\n";

        for( auto i : expected_values )
        {
            csv << rs2_option( i.first ) << "," << i.second << "\n";
        }
    } );

    csv.close();
}

void check_presets_values( rs2::depth_sensor & depth_sens,
                           preset_to_expected_values_map & preset_to_expected_values )
{
    REQUIRE_NOTHROW( depth_sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode ) );
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)preset );

        if( preset == RS2_L500_VISUAL_PRESET_CUSTOM )
            return;

        auto expected_values = preset_to_expected_values[{ rs2_l500_visual_preset( preset ),
                                                           rs2_sensor_mode( mode ) }];
        CAPTURE( rs2_l500_visual_preset( preset ) );
        CAPTURE( rs2_sensor_mode( mode ) );
        for( auto i : expected_values )
        {
            CAPTURE( rs2_option( i.first ) );
            CHECK( depth_sens.get_option( rs2_option( i.first ) ) == i.second );
        }
    } );
}

void check_custom( rs2::depth_sensor & depth_sens ) {}

void build_new_device_an_do( std::function< void( rs2::depth_sensor & depth_sens ) > action )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    action( depth_sens );
}
