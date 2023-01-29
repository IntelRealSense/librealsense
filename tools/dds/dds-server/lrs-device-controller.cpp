// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "lrs-device-controller.h"

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>

#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>

#include <algorithm>
#include <iostream>

using nlohmann::json;
using namespace realdds;
using tools::lrs_device_controller;


#define NAME2SERVER( X )                                                                                               \
    if( server )                                                                                                       \
    {                                                                                                                  \
        if( strcmp( server->type_string(), #X ) )                                                                      \
        {                                                                                                              \
            LOG_ERROR( #X " profile type on a stream '" << stream_name << "' that already has type "                   \
                                                        << server->type_string() );                                    \
            return;                                                                                                    \
        }                                                                                                              \
    }                                                                                                                  \
    else                                                                                                               \
    {                                                                                                                  \
        server = std::make_shared< realdds::dds_##X##_stream_server >( stream_name, sensor_name );                     \
    }                                                                                                                  \
    break

realdds::dds_option_range to_realdds( const rs2::option_range & range )
{
    realdds::dds_option_range ret_val;
    ret_val.max = range.max;
    ret_val.min = range.min;
    ret_val.step = range.step;
    ret_val.default_value = range.def;

    return ret_val;
}

realdds::video_intrinsics to_realdds( const rs2_intrinsics & intr )
{
    realdds::video_intrinsics ret;

    ret.width = intr.width;
    ret.height = intr.height;
    ret.principal_point_x = intr.ppx;
    ret.principal_point_y = intr.ppy;
    ret.focal_lenght_x = intr.fx;
    ret.focal_lenght_y = intr.fy;
    ret.distortion_model = intr.model;
    memcpy( ret.distortion_coeffs.data(), intr.coeffs, sizeof( ret.distortion_coeffs ) );

    return ret;
}

realdds::motion_intrinsics to_realdds( const rs2_motion_device_intrinsic & rs2_intr )
{
    realdds::motion_intrinsics intr;

    memcpy( intr.data.data(), rs2_intr.data, sizeof( intr.data ) );
    memcpy( intr.noise_variances.data(), rs2_intr.noise_variances, sizeof( intr.noise_variances ) );
    memcpy( intr.bias_variances.data(), rs2_intr.bias_variances, sizeof( intr.bias_variances ) );

    return intr;
}

realdds::extrinsics to_realdds( const rs2_extrinsics & rs2_extr )
{
    realdds::extrinsics extr;

    memcpy( extr.rotation.data(), rs2_extr.rotation, sizeof( extr.rotation ) );
    memcpy( extr.translation.data(), rs2_extr.translation, sizeof( extr.translation ) );

    return extr;
}

std::vector< std::shared_ptr< realdds::dds_stream_server > > lrs_device_controller::get_supported_streams()
{
    std::map< std::string, realdds::dds_stream_profiles > stream_name_to_profiles;
    std::map< std::string, int > stream_name_to_default_profile;
    std::map< std::string, std::string > stream_name_to_sensor_name;
    std::map< std::string, std::shared_ptr< realdds::dds_stream_server > > stream_name_to_server;
    std::map< std::string, std::set< realdds::video_intrinsics > > stream_name_to_video_intrinsics;
    std::map< std::string, realdds::motion_intrinsics > stream_name_to_motion_intrinsics;

    //Iterate over all profiles of all sensors and build appropriate dds_stream_servers
    for( auto sensor : _rs_dev.query_sensors() )
    {
        std::string const sensor_name = sensor.get_info( RS2_CAMERA_INFO_NAME );
        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(), stream_profiles.end(), [&]( const rs2::stream_profile & sp ) {
            std::string stream_name = sp.stream_name();
            stream_name_to_sensor_name[stream_name] = sensor_name;

            //Create a realdds::dds_stream_server object for each unique profile type+index
            auto & server = stream_name_to_server[stream_name];
            switch( sp.stream_type() )
            {
            case RS2_STREAM_DEPTH: NAME2SERVER( depth );
            case RS2_STREAM_INFRARED: NAME2SERVER( ir );
            case RS2_STREAM_COLOR: NAME2SERVER( color );
            case RS2_STREAM_FISHEYE: NAME2SERVER( fisheye );
            case RS2_STREAM_CONFIDENCE: NAME2SERVER( confidence );
            case RS2_STREAM_ACCEL: NAME2SERVER( accel );
            case RS2_STREAM_GYRO: NAME2SERVER( gyro );
            case RS2_STREAM_POSE: NAME2SERVER( pose );
            default:
                LOG_ERROR( "unsupported stream type " << sp.stream_type() );
                return;
            }

            //Create appropriate realdds::profile for each sensor profile and map to a stream
            auto & profiles = stream_name_to_profiles[stream_name];
            std::shared_ptr< realdds::dds_stream_profile > profile;
            if( sp.is< rs2::video_stream_profile >() )
            {
                auto const & vsp = sp.as< rs2::video_stream_profile >();
                profile = std::make_shared< realdds::dds_video_stream_profile >(
                    static_cast<int16_t>( vsp.fps() ),
                    realdds::dds_stream_format::from_rs2( vsp.format() ),
                    static_cast< uint16_t >( vsp.width() ),
                    static_cast< int16_t >( vsp.height() ) );
                try
                {
                    auto intr = to_realdds( vsp.get_intrinsics() );
                    stream_name_to_video_intrinsics[stream_name].insert( intr );
                }
                catch( ... ) {} //Some profiles don't have intrinsics
            }
            else if( sp.is< rs2::motion_stream_profile >() )
            {
                const auto & msp = sp.as< rs2::motion_stream_profile >();
                profile = std::make_shared< realdds::dds_motion_stream_profile >(
                    static_cast< int16_t >( msp.fps() ),
                    realdds::dds_stream_format::from_rs2( msp.format() ) );

                stream_name_to_motion_intrinsics[stream_name] = to_realdds( msp.get_motion_intrinsics() );
            }
            else
            {
                LOG_ERROR( "unknown profile type of uid " << sp.unique_id() );
                return;
            }
            if( sp.is_default() )
                stream_name_to_default_profile[stream_name] = static_cast< int >( profiles.size() );
            profiles.push_back( profile );
            LOG_DEBUG( stream_name << ": " << profile->to_string() );
        } );
    }

    //Iterate over the mapped streams and initialize 
    std::vector< std::shared_ptr< realdds::dds_stream_server > > servers;
    for( auto & it : stream_name_to_profiles )
    {
        auto const & stream_name = it.first;
        
        int default_profile_index = 0;
        auto default_profile_it = stream_name_to_default_profile.find( stream_name );
        if( default_profile_it != stream_name_to_default_profile.end() )
            default_profile_index = default_profile_it->second;

        auto const& profiles = it.second;
        if( profiles.empty() )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no profiles" );
            continue;
        }
        auto server = stream_name_to_server[stream_name];
        if( ! server )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no server" );
            continue;
        }
        server->init_profiles( profiles, default_profile_index );
        
        //Set stream intrinsics
        auto video_server = std::dynamic_pointer_cast< dds_video_stream_server >( server );
        auto motion_server = std::dynamic_pointer_cast< dds_motion_stream_server >( server );
        if( video_server )
        {
            video_server->set_intrinsics( std::move( stream_name_to_video_intrinsics[stream_name] ) );
        }
        if( motion_server )
        {
            motion_server->set_intrinsics( std::move( stream_name_to_motion_intrinsics[stream_name] ) );
        }

        realdds::dds_options options;
        //Get supported options for this stream
        for( auto sensor : _rs_dev.query_sensors() )
        {
            std::string const sensor_name = sensor.get_info( RS2_CAMERA_INFO_NAME );
            if( server->sensor_name().compare( sensor_name ) == 0 )
            {
                //Get all sensor supported options
                auto supported_options = sensor.get_supported_options();
                //Hack - some options can be queried only if streaming so start sensor and close after query
                //sensor.open( sensor.get_stream_profiles()[0] );
                //sensor.start( []( rs2::frame f ) {} );
                for( auto option : supported_options )
                {
                    auto dds_opt = std::make_shared< realdds::dds_option >( std::string( sensor.get_option_name( option ) ),
                                                                            server->name() );
                    try
                    {
                        dds_opt->set_value( sensor.get_option( option ) );
                        dds_opt->set_range( to_realdds( sensor.get_option_range( option ) ) );
                        dds_opt->set_description( sensor.get_option_description( option ) );
                    }
                    catch( ... )
                    {
                        LOG_ERROR( "Cannot query details of option " << option );
                        continue; //Some options can be queried only if certain conditions exist skip them for now
                    }
                    options.push_back( dds_opt ); //TODO - filter options relevant for stream type
                }
                //sensor.stop();
                //sensor.close();
            }
        }
        server->init_options( options );
        servers.push_back( server );
    }
    return servers;
}

#undef NAME2SERVER

extrinsics_map get_extrinsics_map( const rs2::device & dev )
{
    extrinsics_map ret;
    std::map< std::string, rs2::stream_profile > stream_name_to_rs2_stream_profile;

    //Iterate over profiles of all sensors and split to streams
    for( auto sensor : dev.query_sensors() )
    {
        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(), stream_profiles.end(), [&]( const rs2::stream_profile & sp ) {
            std::string stream_name = sp.stream_name();
            if( stream_name_to_rs2_stream_profile.count( stream_name ) == 0 )
                stream_name_to_rs2_stream_profile[stream_name] = sp; // Any profile of this stream will do, take the first
        } );
    }

    //For each stream, get extrinsics to all other streams
    for( auto & from : stream_name_to_rs2_stream_profile )
    {
        auto const & from_stream_name = from.first;
        for( auto & to : stream_name_to_rs2_stream_profile )
        {
            auto & to_stream_name = to.first;
            if( from_stream_name != to_stream_name )
            {
                //Get rs2::stream_profile objects for get_extrinsics API call
                const rs2::stream_profile & from_profile = from.second;
                const rs2::stream_profile & to_profile = to.second;
                const auto & extrinsics = from_profile.get_extrinsics_to( to_profile );
                ret[std::make_pair( from_stream_name, to_stream_name )] =
                    std::make_shared< realdds::extrinsics >( to_realdds( extrinsics ) );
            }
        }
    }

    return ret;
}


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

    // Create a supported streams list for initializing the relevant DDS topics
    auto supported_streams = get_supported_streams();

    auto extrinsics = get_extrinsics_map( dev );

    //TODO - get all device level options
    realdds::dds_options options;

    // Initialize the DDS device server with the supported streams
    _dds_device_server->init( supported_streams, options, extrinsics );
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
        json metadata = json( { { "frame_id", f.get_frame_number() } } ); // Special data that is always expected by realdds
        for( size_t i = 0; i < static_cast< size_t >( RS2_FRAME_METADATA_COUNT ); ++i )
        {
            rs2_frame_metadata_value val = static_cast< rs2_frame_metadata_value >( i );
            if( f.supports_frame_metadata( val ) )
            {
                metadata[rs2_frame_metadata_to_string( val )] = f.get_frame_metadata( val );
            }
        }
        _dds_device_server->publish_image( f.get_profile().stream_name(), static_cast< const uint8_t * >( f.get_data() ),
                                           f.get_data_size(), metadata );
    } );
    if( realdds_streams_to_start.size() == 1 )
        std::cout << realdds_streams_to_start[0].first << " stream started" << std::endl;
    else
    {
        std::cout << "[\"";
        size_t i = 0;
        for( ; i < realdds_streams_to_start.size() - 1; ++i )
            std::cout << realdds_streams_to_start[i].first << "\", \"";
        std::cout << realdds_streams_to_start[i].first << "\"] streams started" << std::endl;
    }
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
