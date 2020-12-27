// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <concurrency.h>
#include <types.h>
#include "l500/l500-private.h"
#include "l500/l500-options.h"
#include "hw-monitor.h"

using namespace rs2;

typedef std ::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >,
                   std::map< rs2_option, float > >
    preset_to_expected_values_map;

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
        = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, RS2_DIGITAL_GAIN_HIGH },
            { RS2_L500_VISUAL_PRESET_MAX_RANGE, RS2_DIGITAL_GAIN_HIGH },
            { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, RS2_DIGITAL_GAIN_LOW },
            { RS2_L500_VISUAL_PRESET_SHORT_RANGE, RS2_DIGITAL_GAIN_LOW } };


    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, (float)preset_to_gain[preset] );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode );

        auto laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER );
        std::map< rs2_l500_visual_preset, float > preset_to_laser
            = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, laser_range.def },
                { RS2_L500_VISUAL_PRESET_MAX_RANGE, laser_range.max },
                { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, laser_range.max },
                { RS2_L500_VISUAL_PRESET_SHORT_RANGE, laser_range.def } };

        for( auto dependent_option : preset_dependent_options )
        {
            preset_to_expected_values[{ preset, mode }][dependent_option]
                = depth_sens.get_option_range( dependent_option ).def;
        }
        preset_to_expected_values[{ preset, mode }][RS2_OPTION_DIGITAL_GAIN]
            = (float)preset_to_gain[preset];

        preset_to_expected_values[{ preset, mode }][RS2_OPTION_LASER_POWER]
            = preset_to_laser[preset];
    } );

    return preset_to_expected_values;
}

const int MAX_PARAMS = 4;

struct hw_monitor_command
{
    unsigned int cmd;
    int params[MAX_PARAMS] = { 0 };
};

std::vector< uint8_t > send_command_and_check( rs2::debug_protocol dp,
                                               hw_monitor_command command,
                                               uint32_t expected_size_return = 0 )
{
    const int MAX_HW_MONITOR_BUFFER_SIZE = 1024;

    std::vector< uint8_t > res( MAX_HW_MONITOR_BUFFER_SIZE , 0);
    int size = 0;

    librealsense::hw_monitor::fill_usb_buffer( command.cmd,
                                               command.params[0],
                                               command.params[1],
                                               command.params[2],
                                               command.params[3],
                                               nullptr,
                                               0,
                                               res.data(),
                                               size );

    res = dp.send_and_receive_raw_data( res );
    REQUIRE( res.size() == sizeof( unsigned int ) * ( expected_size_return + 1 ) );  // opcode

    auto vals = reinterpret_cast< int32_t * >( (void *)res.data() );
    REQUIRE( vals[0] == command.cmd );
    res.erase( res.begin(), res.begin() + sizeof( unsigned int ) );  // remove opcode

    return res;
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
                                               (int)mode };


            const uint32_t digital_gain_num_of_values = RS2_DIGITAL_GAIN_LOW;
            auto res = send_command_and_check( dp, command, digital_gain_num_of_values );

            REQUIRE( ds.supports( RS2_OPTION_DIGITAL_GAIN ) );
            float digital_gain_val;

            REQUIRE_NOTHROW( digital_gain_val = ds.get_option( RS2_OPTION_DIGITAL_GAIN ) );

            auto vals = reinterpret_cast< int32_t * >( (void *)res.data() );

            REQUIRE( digital_gain_val >= RS2_DIGITAL_GAIN_HIGH );
            REQUIRE( digital_gain_val <= RS2_DIGITAL_GAIN_LOW );

            option_to_defaults[op.first] = float( vals[(int)digital_gain_val - 1] );
        }
        else
        {
            // FW versions older than get_default doesn't support
            // the way to get the default is set -1 and query
            auto get_command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCGET,
                                                   librealsense::l500_control( op.second ),
                                                   librealsense::l500_command::get_current,
                                                   (int)mode };


            auto res = send_command_and_check( dp, get_command, 1 );

            auto current = *reinterpret_cast< int32_t * >( (void *)res.data() );

            auto set_command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCSET,
                                                   librealsense::l500_control( op.second ),
                                                   -1 };

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
                           preset_to_expected_values_map & preset_to_expected_values )
{
    REQUIRE_NOTHROW( depth_sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    std::ofstream csv( "presets.csv" );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        auto expected_values
            = preset_to_expected_values[{ rs2_l500_visual_preset( preset ), mode }];

        csv << "-----" << preset << " " << mode 
            << "-----"
            << "\n";

        for( auto i : expected_values )
        {
            csv << i.first << "," << i.second << "\n";
        }
    } );

    csv.close();
}

void set_option_values( const rs2::sensor & sens, std::map< rs2_option, float > option_values )
{
    for( auto option_value : option_values )
    {
        REQUIRE_NOTHROW( sens.set_option( option_value.first, option_value.second ) );
        float value;
        REQUIRE_NOTHROW( value = sens.get_option( option_value.first ) );
        REQUIRE( value == option_value.second );
    }
}

void check_presets_values( const rs2::sensor & sens,
                           preset_to_expected_values_map & preset_to_expected_values )
{
    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for_each_preset_mode_combination( [&]( rs2_l500_visual_preset preset, rs2_sensor_mode mode ) {
        REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode ) );
        REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset ) );
        CHECK( sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)preset );

        if( preset == RS2_L500_VISUAL_PRESET_CUSTOM )
            return;

        const std::map< rs2_option, float > & expected_values
            = preset_to_expected_values[{ preset ,
                                           mode }];

        CAPTURE( preset );
        CAPTURE( mode );
        for( auto i : expected_values )
        {
            CAPTURE( i.first );
            CHECK( sens.get_option( i.first ) == i.second );
        }
    } );
}

void build_new_device_and_do( std::function< void( rs2::depth_sensor & depth_sens ) > action )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    action( depth_sens );
}

void check_preset_is_equal_to( rs2::depth_sensor & depth_sens, rs2_l500_visual_preset preset )
{
    if( preset == RS2_L500_VISUAL_PRESET_SHORT_RANGE )
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET )
               == RS2_L500_VISUAL_PRESET_LOW_AMBIENT );  // RS2_L500_VISUAL_PRESET_SHORT_RANGE
                                                         // is the same as
                                                         // RS2_L500_VISUAL_PRESET_LOW_AMBIENT
    else
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ) == preset );
}
