// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <concurrency.h>
#include "../src/types.h"
#include "../src/l500/l500-private.h"
#include "../src/l500/l500-options.h"

using namespace rs2;

#define preset_to_expected_values_map                                                              \
    std ::map< std::pair< rs2_l500_visual_preset, rs2_sensor_mode >, std::map< rs2_option, float > >

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
            = preset_to_gain[preset];

        preset_to_expected_values[{ preset, mode }][RS2_OPTION_LASER_POWER]
            = preset_to_laser[preset];
    } );

    return preset_to_expected_values;
}

const int MAX_PARAMS = 4;
#pragma pack( push, 1 )
struct hw_monitor_command
{
    unsigned int cmd;
    int params[MAX_PARAMS] = { 0 };
};
#pragma pack( pop )

inline std::vector< uint8_t > encode_raw_data_command( const hw_monitor_command & command )
{
    std::vector< uint8_t > res;
    const uint16_t pre_header_data = 0xcdab;  // magic number
    auto header_size = sizeof( unsigned int );
    res.resize( sizeof( hw_monitor_command )
                + header_size );  // TODO: resize std::vector to the right size
    auto write_ptr = res.data();

    auto cur_index = sizeof( uint16_t );
    *reinterpret_cast< uint16_t * >( write_ptr + cur_index ) = pre_header_data;
    cur_index += sizeof( uint16_t );
    *reinterpret_cast< unsigned int * >( write_ptr + cur_index ) = command.cmd;
    cur_index += sizeof( unsigned int );

    for( auto param_index = 0; param_index < MAX_PARAMS; ++param_index )
    {
        *reinterpret_cast< int * >( write_ptr + cur_index ) = command.params[param_index];
        cur_index += sizeof( int );
    }


    *reinterpret_cast< uint16_t * >( write_ptr ) = cur_index - header_size;

    return res;
}

std::vector< uint8_t > send_command_and_check( rs2::debug_protocol dp, hw_monitor_command command,
                                               uint32_t expected_size_return = 1 )
{
    auto res = dp.send_and_receive_raw_data( encode_raw_data_command( command ) );
    REQUIRE( res.size() == sizeof( unsigned int ) * ( expected_size_return + 1 ) );  // opcode

    auto vals = reinterpret_cast< int32_t * >( (void *)res.data() );
    REQUIRE( vals[0] == command.cmd );
    res.erase( res.begin(), res.begin() + sizeof( unsigned int ) ); //remove opcode
    
    return res;
}

    std::map< rs2_option, float > get_expected_default_values( rs2::device & dev )
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
            hw_monitor_command command = { librealsense::ivcam2::fw_cmd::AMCGET,
                                           librealsense::l500_control( op.second ),
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
        //else
        //{
        //    // FW versions older than get_default doesn't support
        //    // the way to get the default is set -1 and query
        //    hw_monitor_command command = { librealsense::ivcam2::fw_cmd::AMCGET,
        //                                   librealsense::l500_control( op.second ),
        //                                   librealsense::l500_command::get_current,
        //                                   (int)mode };


        //    auto res = dp.send_and_receive_raw_data( encode_raw_data_command( command ) );
        //    REQUIRE( res.size() == sizeof( uint32_t ) * ( 2 ) );  // res + opcode
        //    REQUIRE( res[0] == command.cmd );
        //    auto current = reinterpret_cast< int32_t * >( (void *)res.data() );

        //    command
        //        = { librealsense::ivcam2::fw_cmd::AMCSET, librealsense::l500_control( op.second ) };

        //    auto res = dp.send_and_receive_raw_data( encode_raw_data_command( command ) );
        //    _hw_monitor->send( command{ AMCSET, _type, -1 } );

        //    // if the sensor is streaming the value of control will update only whan the next
        //    // frame
        //    // arrive
        //    if( _l500_dev->get_depth_sensor().is_streaming() )
        //        std::this_thread::sleep_for( std::chrono::seconds( 50 ) );

        //    auto def = query( get_current, int( _resolution->query() ) );

        //    _hw_monitor->send( command{ AMCSET, _type, (int)current } );

        //    return def;
        //}
    }


    return option_to_defaults;
}

std::map< rs2_option, float > get_actual_default_values( rs2::depth_sensor & depth_sens )
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

void set_values_manually( rs2::depth_sensor & depth_sens, std::map< rs2_option, float > values )
{
    for( auto val : values )
    {
        REQUIRE( depth_sens.supports( val.first ) );
        REQUIRE_NOTHROW( depth_sens.set_option( val.first, val.second ) );
        REQUIRE( depth_sens.get_option( val.first ) == val.second );
    }
}

void check_presets_values( rs2::depth_sensor & depth_sens,
                           preset_to_expected_values_map & preset_to_expected_values )
{
    REQUIRE( depth_sens.supports( RS2_OPTION_VISUAL_PRESET ) );

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

void validate_presets_value( rs2::depth_sensor & depth_sens, rs2_l500_visual_preset preset )
{
    if( preset == RS2_L500_VISUAL_PRESET_SHORT_RANGE )
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET )
               == RS2_L500_VISUAL_PRESET_LOW_AMBIENT );  // RS2_L500_VISUAL_PRESET_SHORT_RANGE
                                                         // is the same as
                                                         // RS2_L500_VISUAL_PRESET_LOW_AMBIENT
    else
        CHECK( depth_sens.get_option( RS2_OPTION_VISUAL_PRESET ) == preset );
}