// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "lrs-device-controller.h"

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/json.h>

#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>

#include <algorithm>
#include <iostream>

using nlohmann::json;
using namespace realdds;
using namespace tools;


std::shared_ptr< dds_stream_profile > create_dds_stream_profile( rs2_stream type, nlohmann::json const & j )
{
    switch ( type )
    {
    case RS2_STREAM_DEPTH:
    case RS2_STREAM_COLOR:
    case RS2_STREAM_INFRARED:
    case RS2_STREAM_FISHEYE:
    case RS2_STREAM_CONFIDENCE:
        return std::dynamic_pointer_cast< dds_stream_profile >( dds_stream_profile::from_json< dds_video_stream_profile>( j ) );
    case RS2_STREAM_GYRO:
    case RS2_STREAM_ACCEL:
    case RS2_STREAM_POSE:
        return std::dynamic_pointer_cast< dds_stream_profile >( dds_stream_profile::from_json< dds_motion_stream_profile >( j ) );
    }

    throw std::runtime_error( "Unsupported stream type" );
}

rs2_stream stream_name_to_type( std::string const & type_string )
{
    static const std::map< std::string, rs2_stream > type_to_rs2 = {
        { "Depth", RS2_STREAM_DEPTH },
        { "Color", RS2_STREAM_COLOR },
        { "Infrared", RS2_STREAM_INFRARED },
        { "Infrared 1", RS2_STREAM_INFRARED },
        { "Infrared 2", RS2_STREAM_INFRARED },
        { "Fisheye", RS2_STREAM_FISHEYE },
        { "Gyro", RS2_STREAM_GYRO },
        { "Accel", RS2_STREAM_ACCEL },
        { "Gpio", RS2_STREAM_GPIO },
        { "Pose", RS2_STREAM_POSE },
        { "Confidence", RS2_STREAM_CONFIDENCE },
    };
    auto it = type_to_rs2.find( type_string );
    if ( it == type_to_rs2.end() )
    {
        LOG_ERROR( "Unknown stream type '" << type_string << "'" );
        return RS2_STREAM_ANY;
    }
    return it->second;
}

int stream_name_to_index( std::string const & type_string )
{
    int index = 0;
    static const std::map< std::string, int > type_to_index = {
        { "Infrared 1", 1 },
        { "Infrared 2", 2 },
    };
    auto it = type_to_index.find( type_string );
    if ( it != type_to_index.end() )
    {
        index = it->second;
    }

    return index;
}

rs2_option option_name_to_type( const std::string & name, const rs2::sensor & sensor )
{
    for( size_t i = 0; i < static_cast< size_t >( RS2_OPTION_COUNT ); i++ )
    {
        if( name.compare( sensor.get_option_name( static_cast< rs2_option >( i ) ) ) == 0 )
        {
            return static_cast< rs2_option >( i );
        }
    }

    throw std::runtime_error( "Option " + name + " type not found" );
}

rs2::stream_profile get_required_profile( const rs2::sensor & sensor,
                                          std::string stream_name,
                                          std::shared_ptr< dds_stream_profile > profile )
{
    auto sensor_stream_profiles = sensor.get_stream_profiles();
    auto profile_iter = std::find_if( sensor_stream_profiles.begin(),
                                      sensor_stream_profiles.end(),
                                      [&]( rs2::stream_profile sp ) {
                                          auto vp = sp.as< rs2::video_stream_profile >();
                                          auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( profile );
                                          bool video_params_match = ( vp && dds_vp ) ?
                                              vp.width() == dds_vp->width() && vp.height() == dds_vp->height() : true;
                                          return sp.stream_type() == stream_name_to_type( stream_name )
                                              && sp.stream_index() == stream_name_to_index( stream_name )
                                              && sp.fps() == profile->frequency()
                                              && sp.format() == profile->format().to_rs2()
                                              && video_params_match;
                                      } );
    if ( profile_iter == sensor_stream_profiles.end() )
    {
        throw std::runtime_error( "Could not find required profile" );
    }

    return *profile_iter;
}

lrs_device_controller::lrs_device_controller( rs2::device dev, std::shared_ptr< realdds::dds_device_server > dds_device_server )
    : _rs_dev( dev )
    , _dds_device_server( dds_device_server )
{
    if ( ! _dds_device_server )
        throw std::runtime_error( "Empty dds_device_server" );

    _dds_device_server->on_open_streams( [&]( const json & msg ) { start_streaming( msg ); } );
    _dds_device_server->on_close_streams( [&]( const json & msg ) { stop_streaming( msg ); } );
    _dds_device_server->on_set_option( [&]( const std::shared_ptr< realdds::dds_option > & option, float value ) {
        set_option( option, value );
    } );
    _dds_device_server->on_query_option( [&]( const std::shared_ptr< realdds::dds_option > & option ) -> float {
        return query_option( option );
    } );

    //query_sensors returns a copy of the sensors, we keep it to use same copy throughout the run time.
    //Otherwise problems could arise like opening streams and they would close at start_streaming scope end.
    _sensors = _rs_dev.query_sensors();

    _device_sn = _rs_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " created" );
}


lrs_device_controller::~lrs_device_controller()
{
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " deleted" );
}

void lrs_device_controller::start_streaming( const json & msg )
{
    auto msg_profiles = msg["stream-profiles"];

    std::vector< rs2::stream_profile > rs_profiles_to_open;
    std::vector< std::pair < std::string, image_header > > realdds_streams_to_start;

    size_t sensor_index = 0;
    for ( auto it = msg_profiles.begin(); it != msg_profiles.end(); ++it )
    {
        //To get actual rs2::stream_profile to open we need to pass an rs2::sensor to `get_required_profile`.
        std::string requested_stream_name = it.key();
        if ( ! find_sensor( requested_stream_name, sensor_index ) )
            throw std::runtime_error( "Could not find sensor to open stream `" + requested_stream_name + "`" );

        //Now that we have the sensor get the rs2::stream_profile
        auto dds_profile = create_dds_stream_profile( stream_name_to_type( requested_stream_name ), it.value() );
        auto rs2_profile = get_required_profile( _sensors[sensor_index],
                                                 requested_stream_name,
                                                 dds_profile );
        rs_profiles_to_open.push_back( rs2_profile );

        auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( dds_profile );
        realdds::image_header header;
        header.format = dds_profile->format().to_rs2();
        header.width = dds_vp ? dds_vp->width() : 0;
        header.height = dds_vp ? dds_vp->height() : 0;
        realdds_streams_to_start.push_back( std::make_pair( requested_stream_name, std::move( header ) ) );
    }

    //Start streaming
    //TODO - currently assumes all streams are from one sensor. Add support for multiple sensors
    _dds_device_server->start_streaming( realdds_streams_to_start );
    _sensors[sensor_index].open( rs_profiles_to_open );
    _sensors[sensor_index].start( [&]( rs2::frame f ) {
        _dds_device_server->publish_image( f.get_profile().stream_name(), static_cast< const uint8_t * >( f.get_data() ), f.get_data_size() );
    } );
    std::cout << realdds_streams_to_start[0].first << " stream started" << std::endl;
}

void lrs_device_controller::stop_streaming( const json & msg )
{
    auto stream_names = msg["stream-names"];

    size_t sensor_index = 0;
    for ( std::string requested_stream_name : stream_names )
    {
        //Find sensor to close
        if ( find_sensor( requested_stream_name, sensor_index ) )
            break; //TODO - currently assumes all streams are from one sensor. Add support for multiple sensors

        if ( sensor_index == _sensors.size() )
            throw std::runtime_error( "Could not find sensor to close `" + requested_stream_name + "`" );
    }

    //Stop streaming
    _sensors[sensor_index].stop();
    _sensors[sensor_index].close();
    _dds_device_server->stop_streaming( stream_names );
    std::cout << _sensors[sensor_index].get_info( RS2_CAMERA_INFO_NAME ) << " closed. Streams requested to stop: " << stream_names << std::endl;
}

void lrs_device_controller::set_option( const std::shared_ptr< realdds::dds_option > & option, float new_value )
{
    size_t sensor_index = 0;
    find_sensor( option->owner_name(), sensor_index );
        
    if( sensor_index == _sensors.size() )
    {
        //TODO - handle device options
        throw std::runtime_error( "Could not find sensor with `" + option->owner_name() + "` stream" );
    }

    rs2_option opt_type = option_name_to_type( option->get_name(), _sensors[sensor_index] );

    _sensors[sensor_index].set_option( opt_type, new_value );
}

float lrs_device_controller::query_option( const std::shared_ptr< realdds::dds_option > & option )
{
    size_t sensor_index = 0;
    find_sensor( option->owner_name(), sensor_index );

    if( sensor_index == _sensors.size() )
    {
        //TODO - handle device options
        throw std::runtime_error( "Could not find sensor with `" + option->owner_name() + "` stream" );
    }

    rs2_option opt_type = option_name_to_type( option->get_name(), _sensors[sensor_index] );

    return _sensors[sensor_index].get_option( opt_type );
}

bool tools::lrs_device_controller::find_sensor( const std::string & requested_stream_name, size_t & sensor_index )
{
    //Iterate over all the sensors, if a sensor have a profile with a name
    //corresponding to the requested, we have found our sensor.
    for ( sensor_index = 0; sensor_index < _sensors.size(); ++sensor_index )
    {
        for ( auto & profile : _sensors[sensor_index].get_stream_profiles() )
        {
            if ( profile.stream_name() == requested_stream_name )
            {
                return true;
            }
        }
    }

    return false;
}
