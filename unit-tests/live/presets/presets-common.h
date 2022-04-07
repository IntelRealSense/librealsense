// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../live-common.h"
#include <librealsense2/utilities/concurrency/concurrency.h>
#include <src/types.h>
#include <src/l500/l500-private.h>
#include <src/l500/l500-options.h>
#include "../send-hw-monitor-command.h"

using namespace rs2;

const librealsense::firmware_version MIN_GET_DEFAULT_FW_VERSION( "1.5.4.0" );

typedef std::pair< rs2_l500_visual_preset, rs2_sensor_mode > preset_mode_pair;

typedef std::map< preset_mode_pair,
    std::map< rs2_option, float > >
    preset_values_map;

typedef std::map< std::pair< rs2_digital_gain, rs2_sensor_mode >, std::map< rs2_option, float > >
    gain_values_map;

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
    default_laser,
    max_laser
};

const std::map< rs2_l500_visual_preset, std::pair< rs2_digital_gain, presets_useful_laser_values > >
    preset_to_gain_and_laser_map
    = { { RS2_L500_VISUAL_PRESET_NO_AMBIENT, { RS2_DIGITAL_GAIN_HIGH, default_laser } },
        { RS2_L500_VISUAL_PRESET_MAX_RANGE, { RS2_DIGITAL_GAIN_HIGH, max_laser } },
        { RS2_L500_VISUAL_PRESET_LOW_AMBIENT, { RS2_DIGITAL_GAIN_LOW, max_laser } },
        { RS2_L500_VISUAL_PRESET_SHORT_RANGE, { RS2_DIGITAL_GAIN_LOW, default_laser } } };

// except from RS2_L500_VISUAL_PRESET_AUTOMATIC and RS2_L500_VISUAL_PRESET_CUSTOM
void for_each_preset_mode_combination(
    std::function< void(preset_mode_pair) > action )
{
    for( int preset = RS2_L500_VISUAL_PRESET_NO_AMBIENT; preset < RS2_L500_VISUAL_PRESET_AUTOMATIC;
         preset++ )
    {
        for( int sensor_mode = RS2_SENSOR_MODE_VGA; sensor_mode < RS2_SENSOR_MODE_QVGA;
             sensor_mode++ )
        {
            action({ rs2_l500_visual_preset(preset), rs2_sensor_mode(sensor_mode) });
        }
    }
}

// except from RS2_L500_VISUAL_PRESET_AUTOMATIC and RS2_L500_VISUAL_PRESET_CUSTOM
void for_each_gain_mode_combination(
    std::function< void( rs2_digital_gain, rs2_sensor_mode ) > action )
{
    for( int gain = RS2_DIGITAL_GAIN_HIGH; gain <= RS2_DIGITAL_GAIN_LOW; gain++ )
    {
        for( int sensor_mode = RS2_SENSOR_MODE_VGA; sensor_mode < RS2_SENSOR_MODE_COUNT;
             sensor_mode++ )
        {
            action( rs2_digital_gain( gain ), rs2_sensor_mode( sensor_mode ) );
        }
    }
}
// Can be called at a point on test we want to be sure we are on preset and not on custom
inline void reset_camera_preset_mode_to_defaults( const rs2::sensor & sens )
{
    REQUIRE_NOTHROW(
        sens.set_option(RS2_OPTION_VISUAL_PRESET, sens.get_option_range(RS2_OPTION_VISUAL_PRESET).def));
    if (!sens.is_option_read_only(RS2_OPTION_SENSOR_MODE)) {
        REQUIRE_NOTHROW(sens.set_option(RS2_OPTION_SENSOR_MODE, sens.get_option_range(RS2_OPTION_SENSOR_MODE).def));
    }
}

inline preset_mode_pair get_camera_preset_mode_defaults(const rs2::sensor & sens)
{
    preset_mode_pair res;

    REQUIRE_NOTHROW(
        res.first = (rs2_l500_visual_preset)(int)sens.get_option_range(RS2_OPTION_VISUAL_PRESET).def);
    REQUIRE_NOTHROW(res.second = (rs2_sensor_mode)(int)sens.get_option_range(RS2_OPTION_SENSOR_MODE).def);

    return res;
}

std::map< rs2_option, float > get_defaults_from_fw( rs2::device & dev )
{
    std::map< rs2_option, float > option_to_defaults;

    auto dp = dev.as< rs2::debug_protocol >();
    REQUIRE( dp );

    auto ds = dev.first< rs2::depth_sensor >();

    REQUIRE( ds.supports( RS2_OPTION_SENSOR_MODE ) );

    rs2_sensor_mode mode;
    REQUIRE_NOTHROW( mode = rs2_sensor_mode( (int)ds.get_option( RS2_OPTION_SENSOR_MODE ) ) );

    for( auto op : option_to_code )
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

    return option_to_defaults;
}

std::map< rs2_option, float > build_preset_to_expected_defaults_map( rs2::device & dev,
                                                                     rs2::depth_sensor & depth_sens,
                                                                     preset_mode_pair preset_mode )
{
    auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;

    depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN,
                           (float)preset_to_gain_and_laser[preset_mode.first].first );
    depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)preset_mode.second );

    return get_defaults_from_fw( dev );
}

preset_values_map build_presets_to_expected_defaults_map( rs2::device & dev,
                                                          rs2::depth_sensor & depth_sens )
{
    preset_values_map preset_to_expected_defaults_values;

    auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;

    for_each_preset_mode_combination( [&]( preset_mode_pair preset_mode ) {
        preset_to_expected_defaults_values[preset_mode]
            = build_preset_to_expected_defaults_map( dev, depth_sens, preset_mode );
    } );

    return preset_to_expected_defaults_values;
}

void build_preset_to_expected_values_map( rs2::device & dev,
                                          rs2::depth_sensor & depth_sens,
                                          preset_mode_pair preset_mode,
                                          std::map< rs2_option, float > & expected_values,
                                          std::map< rs2_option, float > & expected_defs )
{

    auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;

    auto preset = preset_mode.first;
    auto mode = preset_mode.second;

    depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, (float)preset_to_gain_and_laser[preset].first );
    depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode );

    expected_defs = get_defaults_from_fw( dev );

    for( auto dependent_option : preset_dependent_options )
    {
        expected_values[dependent_option] = depth_sens.get_option_range( dependent_option ).def;
    }

    expected_values[RS2_OPTION_DIGITAL_GAIN] = (float)preset_to_gain_and_laser[preset].first;
    auto laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER );


    expected_values[RS2_OPTION_LASER_POWER]
        = preset_to_gain_and_laser[preset].second == default_laser ? laser_range.def
                                                                   : laser_range.max;
}

void build_presets_to_expected_values_defs_map( rs2::device & dev,
                                                rs2::depth_sensor & depth_sens,
                                                preset_values_map & expected_values,
                                                preset_values_map & expected_defs )
{

    for_each_preset_mode_combination( [&]( preset_mode_pair preset_mode ) {
        build_preset_to_expected_values_map( dev,
                                             depth_sens,
                                             preset_mode,
                                             expected_values[preset_mode],
                                             expected_defs[preset_mode] );
    } );
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

gain_values_map build_gain_to_expected_defaults_map( rs2::device & dev,
                                                     rs2::depth_sensor & depth_sens )
{
    gain_values_map gain_to_expected_defaults_values;

    auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;

    for_each_gain_mode_combination( [&]( rs2_digital_gain gain, rs2_sensor_mode mode ) {
        depth_sens.set_option( RS2_OPTION_DIGITAL_GAIN, (float)gain );
        depth_sens.set_option( RS2_OPTION_SENSOR_MODE, (float)mode );

        gain_to_expected_defaults_values[{ gain, mode }] = get_defaults_from_fw( dev );
    } );

    return gain_to_expected_defaults_values;
}

void reset_hw_controls( rs2::device & dev )
{
    auto dp = dev.as< rs2::debug_protocol >();

    for( auto op : option_to_code )
    {
        auto set_command = hw_monitor_command{ librealsense::ivcam2::fw_cmd::AMCSET,
                                               librealsense::l500_control( op.second ),
                                               -1,
                                               0,
                                               0 };
        send_command_and_check( dp, set_command );
    }
}

std::map< rs2_option, float > get_currents_from_lrs( rs2::depth_sensor & depth_sens )
{
    std::map< rs2_option, float > option_to_currents;

    for( auto op : option_to_code )
    {
        REQUIRE( depth_sens.supports( op.first ) );
        float curr;
        REQUIRE_NOTHROW( curr = depth_sens.get_option( op.first ) );
        option_to_currents[op.first] = curr;
    }
    return option_to_currents;
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

    for_each_preset_mode_combination( [&](preset_mode_pair preset_mode) {
        auto expected_values
            = preset_to_expected_values[preset_mode];

        csv << "-----" << preset_mode.first << " " << preset_mode.second << "-----"
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

void compare_expected_currents_to_actual( const rs2::sensor & sens,
                                          const std::map< rs2_option, float > & expected_values )
{
    for( auto & i : expected_values )
    {
        CAPTURE( i.first );
        CHECK( sens.get_option( i.first ) == i.second );
    }
}

void compare_expected_defaults_to_actual( const rs2::sensor & sens,
                                          const std::map< rs2_option, float > & expected_defaults )
{
    for( auto & i : expected_defaults )
    {
        CAPTURE( i.first );
        CHECK( sens.get_option_range( i.first ).def == i.second );
    }
}

void compare_to_actual( const rs2::sensor & sens,
                        const std::map< rs2_option, float > & expected_values,
                        const std::map< rs2_option, float > & expected_defaults )
{
    compare_expected_currents_to_actual( sens, expected_values );
    compare_expected_defaults_to_actual( sens, expected_defaults );
}

void check_preset_values( const rs2::sensor & sens,
                          rs2_l500_visual_preset preset,
                          rs2_sensor_mode mode,
                          const std::map< rs2_option, float > & expected_values,
                          const std::map< rs2_option, float > & expected_defaults )
{

    if( preset == RS2_L500_VISUAL_PRESET_CUSTOM )
        return;

    CAPTURE( preset, mode );
    compare_to_actual( sens, expected_values, expected_defaults );
}

void check_presets_values( std::vector< rs2_l500_visual_preset > presets_to_check,
                           const rs2::sensor & sens,
                           preset_values_map & preset_to_expected_values,
                           preset_values_map & preset_to_expected_defaults,
                           std::function< void( preset_mode_pair ) > do_before_check = nullptr,
                           std::function< void( preset_mode_pair ) > do_after_check = nullptr )
{
    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    for (auto& preset : presets_to_check)
    {
        for (int sensor_mode = RS2_SENSOR_MODE_VGA; sensor_mode < RS2_SENSOR_MODE_QVGA;
            sensor_mode++)
        {
            preset_mode_pair preset_mode = { preset, (rs2_sensor_mode)sensor_mode };
            if (do_before_check)
                do_before_check(preset_mode);

            check_preset_values(sens,
                preset_mode.first,
                preset_mode.second,
                preset_to_expected_values[preset_mode],
                preset_to_expected_defaults[preset_mode]);

            if (do_after_check)
                do_after_check(preset_mode);
        }
    }
}

void start_depth_ir_confidence( const rs2::sensor & sens,
                                rs2_sensor_mode mode = RS2_SENSOR_MODE_VGA )
{
    auto depth = find_profile( sens, RS2_STREAM_DEPTH, mode );
    auto ir = find_profile( sens, RS2_STREAM_INFRARED, mode );
    auto confidence = find_profile( sens, RS2_STREAM_CONFIDENCE, mode );

    REQUIRE_NOTHROW( sens.open( { depth, ir, confidence } ) );
    
    // Wait for the first frame to arrive: until this time, the FW is in an "undefined"
    // streaming state, and we may get weird behavior.
    frame_queue q;
    REQUIRE_NOTHROW(sens.start(q));
    q.wait_for_frame();
}

void stop_depth( const rs2::sensor & sens )
{
    sens.stop();
    sens.close();
}

void set_mode_preset( const rs2::sensor & sens, preset_mode_pair preset_mode)
{
    if (!sens.is_option_read_only(RS2_OPTION_SENSOR_MODE)) {
        REQUIRE_NOTHROW(sens.set_option(RS2_OPTION_SENSOR_MODE, (float)preset_mode.second));
    }
    REQUIRE_NOTHROW( sens.set_option( RS2_OPTION_VISUAL_PRESET, (float)preset_mode.first) );
    CHECK( sens.get_option( RS2_OPTION_VISUAL_PRESET ) == (float)preset_mode.first);
}

void check_presets_values_while_streaming(
    std::vector<rs2_l500_visual_preset> presets_to_check,
    const rs2::sensor & sens,
    preset_values_map & preset_to_expected_values,
    preset_values_map & preset_to_expected_defaults,
    std::function< void( preset_mode_pair ) > do_before_stream_start = nullptr,
    std::function< void( preset_mode_pair ) > do_after_stream_start = nullptr )
{
    REQUIRE( sens.supports( RS2_OPTION_VISUAL_PRESET ) );

    check_presets_values(
        presets_to_check,
        sens,
        preset_to_expected_values,
        preset_to_expected_defaults,
        [&](preset_mode_pair preset_mode) {
            if( do_before_stream_start )
                do_before_stream_start(preset_mode);

            start_depth_ir_confidence( sens, preset_mode.second);

            if( do_after_stream_start )
                do_after_stream_start(preset_mode);
        },
        [&](preset_mode_pair) { stop_depth( sens ); } );
}

void check_preset_is_equal_to( rs2::depth_sensor & depth_sens, rs2_l500_visual_preset preset )
{
    auto curr_preset
        = ( rs2_l500_visual_preset )(int)depth_sens.get_option( RS2_OPTION_VISUAL_PRESET );
    CAPTURE( curr_preset );

    if( curr_preset != preset )
    {
        auto preset_to_gain_and_laser = preset_to_gain_and_laser_map;
        option_range laser_range;
        REQUIRE_NOTHROW( laser_range = depth_sens.get_option_range( RS2_OPTION_LASER_POWER ) );
        CHECK( laser_range.def == laser_range.max );
        CHECK( preset_to_gain_and_laser[preset].first
               == preset_to_gain_and_laser[curr_preset].first );
    }
}

void build_new_device_and_do( std::function< void( rs2::depth_sensor & depth_sens ) > action )
{
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];

    auto depth_sens = dev.first< rs2::depth_sensor >();

    action( depth_sens );
}
