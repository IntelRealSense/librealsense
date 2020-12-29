// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <concurrency.h>
#include <types.h>
#include "l500/l500-private.h"
#include "l500/l500-options.h"

using namespace rs2;

typedef std::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >,
                  std::map< rs2_option, float > >
    preset_values_map;

// These are all the options that are changed when changing a preset
const std::vector< rs2_option > preset_dependent_options = { RS2_OPTION_POST_PROCESSING_SHARPENING,
                                                             RS2_OPTION_PRE_PROCESSING_SHARPENING,
                                                             RS2_OPTION_NOISE_FILTERING,
                                                             RS2_OPTION_CONFIDENCE_THRESHOLD,
                                                             RS2_OPTION_INVALIDATION_BYPASS,
                                                             RS2_OPTION_DIGITAL_GAIN,
                                                             RS2_OPTION_LASER_POWER,
                                                             RS2_OPTION_AVALANCHE_PHOTO_DIODE,
                                                             RS2_OPTION_MIN_DISTANCE };

std::map< rs2_option, librealsense::l500_control > option_to_code = {
    { RS2_OPTION_POST_PROCESSING_SHARPENING,
      librealsense::l500_control::post_processing_sharpness },
    { RS2_OPTION_PRE_PROCESSING_SHARPENING, librealsense::l500_control::pre_processing_sharpness },
    { RS2_OPTION_NOISE_FILTERING, librealsense::l500_control::noise_filtering },
    { RS2_OPTION_CONFIDENCE_THRESHOLD, librealsense::l500_control::confidence },
    { RS2_OPTION_INVALIDATION_BYPASS, librealsense::l500_control::invalidation_bypass },
    { RS2_OPTION_LASER_POWER, librealsense::l500_control::laser_gain },
    { RS2_OPTION_AVALANCHE_PHOTO_DIODE, librealsense::l500_control::apd },
    { RS2_OPTION_MIN_DISTANCE, librealsense::l500_control::min_distance } };

enum presets_useful_laser_values
{
    defualt_laser,
    max_laser
} ;

const std::map< rs2_l500_visual_preset, std::pair< rs2_digital_gain, presets_useful_laser_values > >
    preset_to_gain_and_laser_map
    = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, { RS2_DIGITAL_GAIN_HIGH, defualt_laser } },
        { RS2_L500_VISUAL_PRESET_MAX_RANGE, { RS2_DIGITAL_GAIN_HIGH, max_laser } },
        { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, { RS2_DIGITAL_GAIN_LOW, defualt_laser } },
        { RS2_L500_VISUAL_PRESET_SHORT_RANGE, { RS2_DIGITAL_GAIN_LOW, max_laser } } };

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

preset_values_map build_preset_to_expected_values_map( rs2::depth_sensor & depth_sens )
{
    std::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >, std::map< rs2_option, float > >
        preset_to_expected_values;

    auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN,
                               (float)preset_to_gain_and_laser[preset].first );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode );

        auto laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER );

        for( auto dependent_option : preset_dependent_options )
        {
            preset_to_expected_values[{ preset, mode }][dependent_option]
                = depth_sens.get_option_range( dependent_option ).def;
        }
        preset_to_expected_values[{ preset, mode }][RS2_OPTION_DIGITAL_GAIN]
            = (float)preset_to_gain_and_laser[preset].first;

        preset_to_expected_values[{ preset, mode }][RS2_OPTION_LASER_POWER]
            = preset_to_gain_and_laser[preset].second == defualt_laser ? laser_range.def
                                                                       : laser_range.max;
    } );

    return preset_to_expected_values;
}

std::map< rs2_option, float > get_defaults_from_fw( rs2::device & dev, bool streaming = false )
{
    std::map< rs2_option, float > option_to_defaults;

    auto dp = dev.as< rs2::debug_protocol >();

    REQUIRE( dp );

    auto fw_version = dev.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION );

    auto new_fw = librealsense::firmware_version( fw_version )
               >= librealsense::firmware_version( "1.5.3.0" );

    auto ds = dev.first< rs2::depth_sensor >();

    REQUIRE( ds.supports( RS2_OPTION_SENSOR_MODE ) );

    rs2_sensor_mode mode;

    REQUIRE_NOTHROW( mode = rs2_sensor_mode( (int)ds.get_option( RS2_OPTION_SENSOR_MODE ) ) );

    for( auto op : option_to_code )
    {
        if( new_fw )
        {
            auto command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCGET,
                                               op.second,
                                               librealsense::l500_command::get_default,
                                               (int)mode,
                                               0 };

            auto res = send_command_and_check( dp, command, 1 );

            REQUIRE( ds.supports( RS2_OPTION_DIGITAL_GAIN ) );
            float digital_gain_val;

            REQUIRE_NOTHROW( digital_gain_val = ds.get_option( RS2_OPTION_DIGITAL_GAIN ) );

            auto val = *reinterpret_cast< int32_t * >( (void *)res.data() );

            option_to_defaults[op.first] = float( val );
        }
        else
        {
            // FW versions older than get_default doesn't support
            // the way to get the default is set -1 and query
            auto get_command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCGET,
                                                   librealsense::l500_control( op.second ),
                                                   librealsense::l500_command::get_current,
                                                   (int)mode};


            auto res = send_command_and_check( dp, get_command, 1 );

            auto current = *reinterpret_cast< int32_t * >( (void *)res.data() );

            auto set_command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCSET,
                                                   librealsense::l500_control( op.second ),
                                                   -1,
                                                   0,
                                                   0 };

            send_command_and_check( dp, set_command );

            // if the sensor is streaming the value of control will update only whan the next
            // frame arrive
            if( streaming )
                std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );

            res = send_command_and_check( dp, get_command, 1 );
            auto def = *reinterpret_cast< int32_t * >( (void *)res.data() );

            set_command = { librealsense::ivcam2::fw_cmd::AMCSET,
                            librealsense::l500_control( op.second ),
                            current };

            send_command_and_check( dp, set_command );

            option_to_defaults[op.first] = float( def );
        }
    }

    return option_to_defaults;
}

std::map< rs2_option, float > get_defaults_from_lrs( rs2::depth_sensor & depth_sens )
{
    std::map< rs2_option, float > option_to_defaults;

    for( auto op : option_to_code )
    {
        REQUIRE( depth_sens.supports( op.first ) );
        option_range range;
        REQUIRE_NOTHROW( range = depth_sens.get_option_range( op.first ) );
        option_to_defaults[op.first] = range.def;
    }
    return option_to_defaults;
}

void compare( const std::map< rs2_option, float > & first,
              const std::map< rs2_option, float > & second )
{
    CAPTURE( first.size() );
    CAPTURE( second.size() );
    REQUIRE( first.size() == second.size() );

    // an order map - should be sorted
    for( auto it_first = first.begin(), it_second = second.begin(); it_first != first.end();
         it_first++, it_second++ )
    {
        REQUIRE( it_first->first == it_second->first );
        REQUIRE( it_first->second == it_second->second );
    }
}

void print_presets_to_csv( rs2::depth_sensor & depth_sens,
                           preset_values_map & preset_to_expected_values )
{
    REQUIRE_NOTHROW( depth_sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    std::ofstream csv( "presets.csv" );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        auto expected_values
            = preset_to_expected_values[{ rs2_l500_visual_preset( preset ), mode }];

        csv << "-----" << preset << " " << mode << "-----"
            << "\n";

        for( auto i : expected_values )
        {
            csv << i.first << "," << i.second << "\n";
        }
    } );

    csv.close();
}

void set_option_values( const rs2::sensor & sens,
                        const std::map< rs2_option, float > & option_values )
{
    for( auto option_value : option_values )
    {
        REQUIRE_NOTHROW( sens.set_option( option_value.first, option_value.second ) );
        float value;
        REQUIRE_NOTHROW( value = sens.get_option( option_value.first ) );
        REQUIRE( value == option_value.second );
    }
}

void check_preset_values( const rs2::sensor & sens,
                          rs2_l500_visual_preset preset,
                          rs2_sensor_mode mode,
                          const std::map< rs2_option, float > & expected_values )
{
    REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode ) );
    REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
    CHECK( sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)preset );
    std::this_thread::sleep_for( std::chrono::milliseconds( 60 ) );
    if( preset == RS2_L500_VISUAL_PRESET_CUSTOM )
        return;

    CAPTURE( preset );
    CAPTURE( mode );
    for( auto & i : expected_values )
    {
        CAPTURE( i.first );
        CHECK( sens.get_option( i.first ) == i.second );
    }
}

void check_presets_values( const rs2::sensor & sens,
                           preset_values_map & preset_to_expected_values )
{
    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        check_preset_values( sens, preset, mode, preset_to_expected_values[{ preset, mode }] );
    } );
}

void check_presets_values_while_streaming(
    const rs2::sensor & sens, preset_values_map & preset_to_expected_values )
{
    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        auto depth = find_profile( sens, RS2_STREAM_DEPTH, mode );
        auto ir = find_profile( sens, RS2_STREAM_INFRARED, mode );
        auto confidence = find_profile( sens, RS2_STREAM_CONFIDENCE, mode );

        do_while_streaming( sens, { depth, ir, confidence }, [&]() {
            check_preset_values( sens, preset, mode, preset_to_expected_values[{ preset, mode }] );
        } );
    } );
}

void check_preset_is_equal_to( rs2::depth_sensor & depth_sens, rs2_l500_visual_preset preset )
{
    auto curr_preset = (rs2_l500_visual_preset)(int)depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ); 
    if( curr_preset != preset )
    {
        auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;
        option_range laser_range;
        REQUIRE_NOTHROW(laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER ));
        CHECK( laser_range.def == laser_range.max );
        CHECK( preset_to_gain_and_laser[preset].first
               == preset_to_gain_and_laser[curr_preset].first );
    }
}
