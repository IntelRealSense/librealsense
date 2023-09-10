// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include "lrs-device-controller.h"

#include <common/metadata-helper.h>

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>

#include <realdds/topics/image-msg.h>
#include <realdds/topics/imu-msg.h>
#include <realdds/topics/ros2/ros2vector3.h>
#include <realdds/topics/flexible-msg.h>
#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>

#include <algorithm>
#include <iostream>

using nlohmann::json;
using namespace realdds;
using tools::lrs_device_controller;


#define CREATE_SERVER_IF_NEEDED( X )                                                                                   \
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


static std::string stream_name_from_rs2( const rs2::stream_profile & profile )
{
    // ROS stream names cannot contain spaces! We use underscores instead:
    std::string stream_name;
    switch( profile.stream_type() )
    {
    case RS2_STREAM_ACCEL:
    case RS2_STREAM_GYRO:
        stream_name = "Motion";
        break;
    default:
        stream_name = rs2_stream_to_string( profile.stream_type() );
        if( profile.stream_index() )
            stream_name += '_' + std::to_string( profile.stream_index() );
        break;
    }
    return stream_name;
}


std::vector< std::shared_ptr< realdds::dds_stream_server > > lrs_device_controller::get_supported_streams()
{
    std::map< std::string, realdds::dds_stream_profiles > stream_name_to_profiles;
    std::map< std::string, size_t > stream_name_to_default_profile;
    std::map< std::string, std::set< realdds::video_intrinsics > > stream_name_to_video_intrinsics;

    // Iterate over all profiles of all sensors and build appropriate dds_stream_servers
    for( auto sensor : _rs_dev.query_sensors() )
    {
        std::string const sensor_name = sensor.get_info( RS2_CAMERA_INFO_NAME );
        // We keep a copy of the sensors throughout the run time:
        // Otherwise problems could arise like opening streams and they would close at on_open_streams scope end.
        _rs_sensors[sensor_name] = sensor;

        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(), stream_profiles.end(), [&]( const rs2::stream_profile & sp )
        {
            std::string stream_name = stream_name_from_rs2( sp );

            //Create a realdds::dds_stream_server object for each unique profile type+index
            auto & server = _stream_name_to_server[stream_name];
            switch( sp.stream_type() )
            {
            case RS2_STREAM_DEPTH: CREATE_SERVER_IF_NEEDED( depth );
            case RS2_STREAM_INFRARED: CREATE_SERVER_IF_NEEDED( ir );
            case RS2_STREAM_COLOR: CREATE_SERVER_IF_NEEDED( color );
            case RS2_STREAM_CONFIDENCE: CREATE_SERVER_IF_NEEDED( confidence );
            case RS2_STREAM_ACCEL:
            case RS2_STREAM_GYRO:
                CREATE_SERVER_IF_NEEDED( motion );
            default:
                LOG_ERROR( "unsupported stream type " << sp.stream_type() );
                return;
            }

            // Create appropriate realdds::profile for each sensor profile and map to a stream
            auto & profiles = stream_name_to_profiles[stream_name];
            std::shared_ptr< realdds::dds_stream_profile > profile;
            bool insert_profile = true;
            if( auto const vsp = rs2::video_stream_profile( sp ) )
            {
                profile = std::make_shared< realdds::dds_video_stream_profile >(
                    static_cast< int16_t >( vsp.fps() ),
                    realdds::dds_video_encoding::from_rs2( vsp.format() ),
                    static_cast< uint16_t >( vsp.width() ),
                    static_cast< int16_t >( vsp.height() ) );
                try
                {
                    auto intr = to_realdds( vsp.get_intrinsics() );
                    stream_name_to_video_intrinsics[stream_name].insert( intr );
                }
                catch( ... ) {} //Some profiles don't have intrinsics
            }
            else if( auto const msp = rs2::motion_stream_profile( sp ) )
            {
                profile = std::make_shared< realdds::dds_motion_stream_profile >(
                    static_cast< int16_t >( msp.fps() ) );

                auto motion_server = std::dynamic_pointer_cast< dds_motion_stream_server >( server );
                if( RS2_STREAM_ACCEL == msp.stream_type() )
                {
                    insert_profile = false;  // We report only Gyro profiles
                    motion_server->set_accel_intrinsics( to_realdds( msp.get_motion_intrinsics() ) );
                }
                else
                    motion_server->set_gyro_intrinsics( to_realdds( msp.get_motion_intrinsics() ) );
            }
            else
            {
                LOG_ERROR( "unknown profile type of uid " << sp.unique_id() );
                return;
            }
            if( insert_profile )
            {
                if( sp.is_default() )
                    stream_name_to_default_profile[stream_name] = profiles.size();
                profiles.push_back( profile );
                LOG_DEBUG( stream_name << ": " << profile->to_string() );
            }
        } );
    }

    override_default_profiles( stream_name_to_profiles, stream_name_to_default_profile );

    // Iterate over the mapped streams and initialize
    std::vector< std::shared_ptr< realdds::dds_stream_server > > servers;
    std::set< std::string > sensors_handled;
    for( auto & it : stream_name_to_profiles )
    {
        auto const & stream_name = it.first;

        size_t default_profile_index = 0;
        auto default_profile_it = stream_name_to_default_profile.find( stream_name );
        if( default_profile_it != stream_name_to_default_profile.end() )
            default_profile_index = default_profile_it->second;
        else
            LOG_ERROR( "no default profile found; using first available in " << stream_name );

        auto const & profiles = it.second;
        if( profiles.empty() )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no profiles" );
            continue;
        }
        auto server = _stream_name_to_server[stream_name];
        if( ! server )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no server" );
            continue;
        }

        if( auto video_server = std::dynamic_pointer_cast< dds_video_stream_server >( server ) )
        {
            video_server->set_intrinsics( std::move( stream_name_to_video_intrinsics[stream_name] ) );
            // Set stream metadata support (currently if the device supports metadata all streams does)
            // Must be done before calling init_profiles()
            if( _md_enabled )
                server->enable_metadata();
        }

        server->init_profiles( profiles, default_profile_index );

        // Get supported options and recommended filters for this stream
        realdds::dds_options stream_options;
        std::vector< std::string > filter_names;
        for( auto sensor : _rs_dev.query_sensors() )
        {
            std::string const sensor_name = sensor.get_info( RS2_CAMERA_INFO_NAME );
            if( server->sensor_name().compare( sensor_name ) != 0 )
                continue;

            // Multiple streams map to the same sensor so we may go over the same sensor multiple times - we
            // only need to do this once per sensor!
            if( sensors_handled.emplace( sensor_name ).second )
            {
                auto supported_options = sensor.get_supported_options();
                // Hack - some options can be queried only if streaming so start sensor and close after query
                // sensor.open( sensor.get_stream_profiles()[0] );
                // sensor.start( []( rs2::frame f ) {} );
                for( auto option_id : supported_options )
                {
                    // Certain options are automatically added by librealsense and shouldn't actually be shared
                    if( option_id == RS2_OPTION_FRAMES_QUEUE_SIZE )
                        continue;  // Added automatically for every sensor_base

                    std::string option_name = sensor.get_option_name( option_id );
                    auto dds_opt = std::make_shared< realdds::dds_option >( option_name );
                    try
                    {
                        dds_opt->set_value( sensor.get_option( option_id ) );
                        dds_opt->set_range( to_realdds( sensor.get_option_range( option_id ) ) );
                        dds_opt->set_description( sensor.get_option_description( option_id ) );
                    }
                    catch( ... )
                    {
                        LOG_ERROR( "Cannot query details of option " << option_id );
                        continue;  // Some options can be queried only if certain conditions exist skip them for now
                    }
                    stream_options.push_back( dds_opt );  // TODO - filter options relevant for stream type
                }
                // sensor.stop();
                // sensor.close();

                auto recommended_filters = sensor.get_recommended_filters();
                for( auto const & filter : recommended_filters )
                    filter_names.push_back( filter.get_info( RS2_CAMERA_INFO_NAME ) );

                server->init_options( stream_options );
                server->set_recommended_filters( std::move( filter_names ) );
            }
        }

        servers.push_back( server );
    }

    return servers;
}

#undef CREATE_SERVER_IF_NEEDED

extrinsics_map get_extrinsics_map( const rs2::device & dev )
{
    extrinsics_map ret;
    std::map< std::string, rs2::stream_profile > stream_name_to_rs2_stream_profile;

    // Iterate over profiles of all sensors and split to streams
    for( auto sensor : dev.query_sensors() )
    {
        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(),
                       stream_profiles.end(),
                       [&]( const rs2::stream_profile & sp )
                       {
                           if( RS2_STREAM_ACCEL == sp.stream_type() )
                               return;  // Ignore the accelerometer; we want the gyro extrinsics!
                           std::string stream_name = stream_name_from_rs2( sp );
                           auto & rs2_stream_profile = stream_name_to_rs2_stream_profile[stream_name];
                           if( ! rs2_stream_profile )
                               rs2_stream_profile = sp;  // Any profile of this stream will do, take the first
                       } );
    }

    // For each stream, get extrinsics to all other streams
    for( auto & from : stream_name_to_rs2_stream_profile )
    {
        auto const & from_stream_name = from.first;
        for( auto & to : stream_name_to_rs2_stream_profile )
        {
            auto & to_stream_name = to.first;
            if( from_stream_name != to_stream_name )
            {
                // Get rs2::stream_profile objects for get_extrinsics API call
                const rs2::stream_profile & from_profile = from.second;
                const rs2::stream_profile & to_profile = to.second;
                const auto & extrinsics = from_profile.get_extrinsics_to( to_profile );
                ret[std::make_pair( from_stream_name, to_stream_name )] =
                    std::make_shared< realdds::extrinsics >( to_realdds( extrinsics ) );
                LOG_DEBUG( "have extrinsics from " << from_stream_name << " to " << to_stream_name );
            }
        }
    }

    return ret;
}


std::shared_ptr< dds_stream_profile > create_dds_stream_profile( std::string const & type_string, nlohmann::json const & j )
{
    if( "motion" == type_string )
        return dds_stream_profile::from_json< dds_motion_stream_profile >( j );

    static const std::set< std::string > video_types = { "depth", "color", "ir", "confidence" };
    if( video_types.find( type_string ) != video_types.end() )
        return dds_stream_profile::from_json< dds_video_stream_profile >( j );

    throw std::runtime_error( "unsupported stream type '" + type_string + "'" );
}


rs2_stream stream_name_to_type( std::string const & type_string )
{
    static const std::map< std::string, rs2_stream > type_to_rs2 = {
        { "Depth", RS2_STREAM_DEPTH },
        { "Color", RS2_STREAM_COLOR },
        { "Infrared", RS2_STREAM_INFRARED },
        { "Infrared_1", RS2_STREAM_INFRARED },
        { "Infrared_2", RS2_STREAM_INFRARED },
        { "Motion", RS2_STREAM_GYRO },  // We report only gyro profiles
        { "Gpio", RS2_STREAM_GPIO },
        { "Confidence", RS2_STREAM_CONFIDENCE },
    };
    auto it = type_to_rs2.find( type_string );
    if( it == type_to_rs2.end() )
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
        { "Infrared_1", 1 },
        { "Infrared_2", 2 },
    };
    auto it = type_to_index.find( type_string );
    if( it != type_to_index.end() )
    {
        index = it->second;
    }

    return index;
}

rs2_option option_name_to_id( const std::string & name )
{
    auto option_id = rs2_option_from_string( name.c_str() );
    if( RS2_OPTION_COUNT == option_id )
        throw std::runtime_error( "Option '" + name + "' not found" );
    return option_id;
}


bool profiles_are_compatible( std::shared_ptr< dds_stream_profile > const & p1,
                              std::shared_ptr< dds_stream_profile > const & p2,
                              bool any_encoding = false )
{
    auto vp1 = std::dynamic_pointer_cast< realdds::dds_video_stream_profile >( p1 );
    auto vp2 = std::dynamic_pointer_cast< realdds::dds_video_stream_profile >( p2 );
    if( ! ! vp1 != ! ! vp2 )
        return false;  // types aren't the same
    if( vp1 && vp2 )
    {
        if( vp1->width() != vp2->width() || vp1->height() != vp2->height() )
            return false;
        if( ! any_encoding && vp1->encoding() != vp2->encoding() )
            return false;
    }
    return p1->frequency() == p2->frequency();
}


rs2::stream_profile get_required_profile( const rs2::sensor & sensor,
                                          std::vector< rs2::stream_profile > const & sensor_stream_profiles,
                                          std::string const & stream_name,
                                          std::shared_ptr< dds_stream_profile > const & profile )
{
    auto const stream_type = stream_name_to_type( stream_name );
    auto const stream_index = stream_name_to_index( stream_name );

    auto profile_iter = std::find_if( sensor_stream_profiles.begin(),
                                      sensor_stream_profiles.end(),
                                      [&]( rs2::stream_profile const & sp ) {
                                          auto vp = sp.as< rs2::video_stream_profile >();
                                          auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( profile );
                                          bool video_params_match = ( vp && dds_vp )
                                                                      ? vp.width() == dds_vp->width()
                                                                            && vp.height() == dds_vp->height()
                                                                            && vp.format() == dds_vp->encoding().to_rs2()
                                                                      : true;
                                          return sp.stream_type() == stream_type
                                              && sp.stream_index() == stream_index
                                              && sp.fps() == profile->frequency()
                                              && video_params_match;
                                      } );
    if( profile_iter == sensor_stream_profiles.end() )
    {
        throw std::runtime_error( "Could not find required profile" );
    }

    return *profile_iter;
}


static std::shared_ptr< realdds::dds_stream_profile >
find_profile( std::shared_ptr< realdds::dds_stream_server > const & stream,
              std::shared_ptr< realdds::dds_stream_profile > const & profile,
              bool any_encoding = false )
{
    auto & stream_profiles = stream->profiles();
    auto it = std::find_if( stream_profiles.begin(),
                            stream_profiles.end(),
                            [&]( std::shared_ptr< realdds::dds_stream_profile > const & sp )
                            {
                                return profiles_are_compatible( sp, profile, any_encoding );
                            } );
    std::shared_ptr< realdds::dds_stream_profile > found_profile;
    if( it != stream_profiles.end() )
        found_profile = *it;
    return found_profile;
}


std::shared_ptr< realdds::dds_stream_server >
lrs_device_controller::frame_to_streaming_server( rs2::frame const & f, rs2::stream_profile * p_profile ) const
{
    rs2::stream_profile profile_;
    if( ! p_profile )
        p_profile = &profile_;
    rs2::stream_profile & profile = *p_profile;

    profile = f.get_profile();
    auto const stream_name = stream_name_from_rs2( profile );
    auto it = _stream_name_to_server.find( stream_name );
    if( it == _stream_name_to_server.end() )
        return {};

    auto & server = it->second;
    if( ! _bridge.is_streaming( server ) )
        return {};

    return server;
}


lrs_device_controller::lrs_device_controller( rs2::device dev, std::shared_ptr< realdds::dds_device_server > dds_device_server )
    : _rs_dev( dev )
    , _dds_device_server( dds_device_server )
{
    if( ! _dds_device_server )
        throw std::runtime_error( "Empty dds_device_server" );

    _dds_device_server->on_set_option( [&]( const std::shared_ptr< realdds::dds_option > & option, float value ) {
        set_option( option, value );
    } );
    _dds_device_server->on_query_option( [&]( const std::shared_ptr< realdds::dds_option > & option ) -> float {
        return query_option( option );
    } );
    _dds_device_server->on_control(
        [this]( std::string const & id, nlohmann::json const & control, nlohmann::json & reply )
        { return on_control( id, control, reply ); } );

    _device_sn = _rs_dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " created" );

    // Some camera models support metadata for frames. can_support_metadata will tell us if this model does.
    // Also, to get the metadata driver support needs to be enabled, requires administrator rights on Windows and Linux.
    // is_enabled will return current state. If one of the conditions is false we cannot get metadata from the device.
    _md_enabled = rs2::metadata_helper::instance().can_support_metadata( _rs_dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE ) )
               && rs2::metadata_helper::instance().is_enabled( _rs_dev.get_info( RS2_CAMERA_INFO_PHYSICAL_PORT ) );

    // Create a supported streams list for initializing the relevant DDS topics
    auto supported_streams = get_supported_streams();

    _bridge.on_start_sensor(
        [this]( std::string const & sensor_name, dds_stream_profiles const & active_profiles )
        {
            auto & sensor = _rs_sensors[sensor_name];
            auto rs2_profiles = get_rs2_profiles( active_profiles );
            sensor.open( rs2_profiles );
            if( sensor.is< rs2::motion_sensor >() )
            {
                struct imu_context
                {
                    realdds::topics::imu_msg message;
                    // We will be getting gyro and accel together, from different threads:
                    // Don't want to change something mid-send
                    std::mutex mutex;
                };
                sensor.start(
                    [this, imu = std::make_shared< imu_context >()]( rs2::frame f )
                    {
                        rs2::stream_profile stream_profile;
                        auto motion = std::dynamic_pointer_cast< realdds::dds_motion_stream_server >(
                            frame_to_streaming_server( f, &stream_profile ) );
                        if( ! motion )
                            return;

                        auto xyz = reinterpret_cast< float const * >( f.get_data() );
                        if( RS2_STREAM_ACCEL == stream_profile.stream_type() )
                        {
                            std::unique_lock< std::mutex > lock( imu->mutex );
                            imu->message.accel_data().x( xyz[0] );  // in m/s^2
                            imu->message.accel_data().y( xyz[1] );
                            imu->message.accel_data().z( xyz[2] );
                            return;  // Don't actually publish
                        }
                        imu->message.gyro_data().x( xyz[0] );  // rad/sec, which is what we need
                        imu->message.gyro_data().y( xyz[1] );
                        imu->message.gyro_data().z( xyz[2] );
                        imu->message.timestamp(  // in sec.nsec
                            static_cast< long double >( f.get_timestamp() ) / 1e3 );
                        std::unique_lock< std::mutex > lock( imu->mutex );
                        motion->publish_motion( std::move( imu->message ) );

                        // motion streams have no metadata!
                    } );
            }
            else
            {
                sensor.start(
                    [this]( rs2::frame f )
                    {
                        auto video = std::dynamic_pointer_cast< realdds::dds_video_stream_server >(
                            frame_to_streaming_server( f ) );
                        if( ! video )
                            return;

                        dds_time const timestamp  // in sec.nsec
                            ( static_cast< long double >( f.get_timestamp() ) / 1e3 );

                        realdds::topics::image_msg image;
                        auto data = static_cast< const uint8_t * >( f.get_data() );
                        image.raw_data.assign( data, data + f.get_data_size() );
                        image.height = video->get_image_header().height;
                        image.width = video->get_image_header().width;
                        image.timestamp = timestamp;
                        video->publish_image( std::move( image ) );

                        publish_frame_metadata( f, timestamp );
                    } );
            }
            std::cout << sensor_name << " sensor started" << std::endl;
        } );
    _bridge.on_stop_sensor(
        [this]( std::string const & sensor_name )
        {
            auto & sensor = _rs_sensors[sensor_name];
            sensor.stop();
            sensor.close();
            std::cout << sensor_name << " sensor stopped" << std::endl;
        } );
    _bridge.on_error(
        [this]( std::string const & error_string )
        {
            nlohmann::json j = nlohmann::json::object( {
                { "id", "error" },
                { "error", error_string },
            } );
            _dds_device_server->publish_notification( std::move( j ) );
        } );
    _bridge.init( supported_streams );

    auto extrinsics = get_extrinsics_map( dev );

    realdds::dds_options options;  // TODO - get all device level options

    // Initialize the DDS device server with the supported streams
    _dds_device_server->init( supported_streams, options, extrinsics );
}


lrs_device_controller::~lrs_device_controller()
{
    LOG_DEBUG( "LRS device manager for device: " << _device_sn << " deleted" );
}


bool lrs_device_controller::on_open_streams( nlohmann::json const & control, nlohmann::json & reply )
{
    // Note that this function is called "start-streaming" but it's really a response to "open-streams" so does not
    // actually start streaming. It simply sets and locks in which streams should be open when streaming starts.
    // This effectively lets one control _specifically_ which streams should be streamable, and nothing else: if left
    // out, a sensor is reset back to its default state using implicit stream selection.
    // (For example, the 'Stereo Module' sensor controls Depth, IR1, IR2: but turning on all 3 has performance
    // implications and may not be desirable. So you can open only Depth and IR1/2 will stay inactive...)
    if( rsutils::json::get< bool >( control, "reset", true ) )
        _bridge.reset();

    auto const & msg_profiles = control["stream-profiles"];
    for( auto const & name2profile : msg_profiles.items() )
    {
        std::string const & stream_name = name2profile.key();
        auto name2server = _stream_name_to_server.find( stream_name );
        if( name2server == _stream_name_to_server.end() )
            throw std::runtime_error( "invalid stream name '" + stream_name + "'" );
        auto server = name2server->second;

        auto requested_profile
            = create_dds_stream_profile( server->type_string(), name2profile.value() );
        auto profile = find_profile( server, requested_profile );
        if( ! profile )
            throw std::runtime_error( "invalid profile " + requested_profile->to_string() + " for stream '"
                                      + stream_name + "'" );

        _bridge.open( profile );
    }

    // We're here so all the profiles were acceptable; lock them in -- with no implicit profiles!
    if( rsutils::json::get< bool >( control, "commit", true ) )
        _bridge.commit();

    // We don't touch the reply - it's already filled in for us
    return true;
}


void lrs_device_controller::publish_frame_metadata( const rs2::frame & f, realdds::dds_time const & timestamp )
{
    if( ! _dds_device_server->has_metadata_readers() )
        return;

    nlohmann::json md_header = nlohmann::json::object( {
        { "frame-number", f.get_frame_number() },               // communicated; up to client to pick up
        { "timestamp", timestamp.to_ns() },                     // syncer key: needs to match the image timestamp, bit-for-bit!
        { "timestamp-domain", f.get_frame_timestamp_domain() }  // needed if we're dealing with different domains!
    } );
    if( f.is< rs2::depth_frame >() )
        md_header["depth-units"] = f.as< rs2::depth_frame >().get_units();

    nlohmann::json metadata = nlohmann::json::object();
    for( size_t i = 0; i < static_cast< size_t >( RS2_FRAME_METADATA_COUNT ); ++i )
    {
        rs2_frame_metadata_value val = static_cast< rs2_frame_metadata_value >( i );
        if( f.supports_frame_metadata( val ) )
            metadata[rs2_frame_metadata_to_string( val )] = f.get_frame_metadata( val );
    }

    nlohmann::json md_msg = nlohmann::json::object( {
        { "stream-name", stream_name_from_rs2( f.get_profile() ) },
        { "header", std::move( md_header ) },
        { "metadata", std::move( metadata ) },
    } );
    _dds_device_server->publish_metadata( std::move( md_msg ) );
}


std::vector< rs2::stream_profile >
lrs_device_controller::get_rs2_profiles( realdds::dds_stream_profiles const & dds_profiles ) const
{
    std::vector< rs2::stream_profile > rs_profiles;
    for( auto & dds_profile : dds_profiles )
    {
        std::string stream_name = dds_profile->stream()->name();
        std::string sensor_name = dds_profile->stream()->sensor_name();

        auto it = _rs_sensors.find( sensor_name );
        if( it == _rs_sensors.end() )
        {
            LOG_ERROR( "invalid sensor name '" << sensor_name << "'" );
            continue;
        }
        auto & sensor = it->second;
        auto const sensor_stream_profiles = sensor.get_stream_profiles();
        auto rs2_profile = get_required_profile( sensor, sensor_stream_profiles, stream_name, dds_profile );
        rs_profiles.push_back( rs2_profile );

        if( rs2_profile.stream_type() == RS2_STREAM_GYRO )
        {
            // When we start the Gyro, we want to start the Accelerometer, too!
            rs2::stream_profile accel_profile;
            assert( accel_profile.fps() == 0 );
            for( auto & sp : sensor_stream_profiles )
            {
                if( sp.stream_type() != RS2_STREAM_ACCEL )
                    continue;
                if( sp.format() != rs2_profile.format() )
                    continue;
                if( sp.fps() > accel_profile.fps() )
                    accel_profile = sp;
            }
            if( accel_profile )
            {
                LOG_DEBUG( "adding accel profile @ " << accel_profile.fps() << " FPS" );
                rs_profiles.push_back( accel_profile );
            }
        }
    }
    return rs_profiles;
}


void lrs_device_controller::set_option( const std::shared_ptr< realdds::dds_option > & option, float new_value )
{
    auto stream = option->stream();
    if( ! stream )
        // TODO device option? not implemented yet
        throw std::runtime_error( "device option not implemented" );
    auto it = _stream_name_to_server.find( stream->name() );
    if( it == _stream_name_to_server.end() )
        throw std::runtime_error( "no stream '" + stream->name() + "' in device" );
    auto server = it->second;
    auto & sensor = _rs_sensors[server->sensor_name()];
    sensor.set_option( option_name_to_id( option->get_name() ), new_value );
}


float lrs_device_controller::query_option( const std::shared_ptr< realdds::dds_option > & option )
{
    auto stream = option->stream();
    if( ! stream )
        // TODO device option? not implemented yet
        throw std::runtime_error( "device option not implemented" );
    auto it = _stream_name_to_server.find( stream->name() );
    if( it == _stream_name_to_server.end() )
        throw std::runtime_error( "no stream '" + stream->name() + "' in device" );
    auto server = it->second;
    auto & sensor = _rs_sensors[server->sensor_name()];
    return sensor.get_option( option_name_to_id( option->get_name() ) );
}


void lrs_device_controller::override_default_profiles( const std::map< std::string, realdds::dds_stream_profiles > & stream_name_to_profiles,
                                                       std::map< std::string, size_t > & stream_name_to_default_profile ) const
{
    // Default resolution for RealSense modules, set according to system SW architect definitions
    std::string const product_line = _rs_dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
    if( product_line == "D400" )
    {
        static const std::string RS405_PID( "0B5B", 4 );
        static const std::string RS410_PID( "0AD2", 4 );
        static const std::string RS415_PID( "0AD3", 4 );

        std::string product_id = _rs_dev.get_info( RS2_CAMERA_INFO_PRODUCT_ID );

        // For best image quality global shutter should use 848x480 resolution, rolling shutter 1280x720
        uint16_t fps = 30;
        uint16_t width = 848;
        uint16_t height = 480;
        if( product_id == RS415_PID || product_id == RS410_PID )
        {
            width = 1280;
            height = 720;
        }
        stream_name_to_default_profile["Depth"] = get_index_of_profile( stream_name_to_profiles.at( "Depth" ),
            realdds::dds_video_stream_profile( fps, realdds::dds_video_encoding::from_rs2( RS2_FORMAT_Z16 ), width, height ) );
        stream_name_to_default_profile["Infrared_1"] = get_index_of_profile( stream_name_to_profiles.at( "Infrared_1" ),
            realdds::dds_video_stream_profile( fps, realdds::dds_video_encoding::from_rs2( RS2_FORMAT_Y8 ), width, height ) );
        stream_name_to_default_profile["Infrared_2"] = get_index_of_profile( stream_name_to_profiles.at( "Infrared_2" ),
            realdds::dds_video_stream_profile( fps, realdds::dds_video_encoding::from_rs2( RS2_FORMAT_Y8 ), width, height ) );

        width = 1280;
        height = 720;
        if( product_id == RS405_PID )
        {
            width = 848;
            height = 480;
        }
        stream_name_to_default_profile["Color"] = get_index_of_profile( stream_name_to_profiles.at( "Color" ),
            realdds::dds_video_stream_profile( fps, realdds::dds_video_encoding::from_rs2( RS2_FORMAT_YUYV ), width, height ) );
    }
}


size_t lrs_device_controller::get_index_of_profile( const realdds::dds_stream_profiles & profiles,
                                                    const realdds::dds_video_stream_profile & profile ) const
{
    for( size_t i = 0; i < profiles.size(); ++i )
    {
        auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( profiles[i] );
        if( dds_vp->frequency() == profile.frequency()
            && dds_vp->encoding() == profile.encoding()
            && dds_vp->width()  == profile.width()
            && dds_vp->height() == profile.height() )
            return i;
    }

    return 0;
}


size_t lrs_device_controller::get_index_of_profile( const realdds::dds_stream_profiles & profiles,
                                                    const realdds::dds_motion_stream_profile & profile ) const
{
    for( size_t i = 0; i < profiles.size(); ++i )
    {
        if( profiles[i]->frequency() == profile.frequency() )
            return i;
    }

    return 0;
}


bool lrs_device_controller::on_control( std::string const & id, nlohmann::json const & control, nlohmann::json & reply )
{
    static std::map< std::string, bool ( lrs_device_controller::* )( nlohmann::json const &, nlohmann::json & ) > const
        control_handlers{
            { "hw-reset", &lrs_device_controller::on_hardware_reset },
            { "open-streams", &lrs_device_controller::on_open_streams },
        };
    auto it = control_handlers.find( id );
    if( it == control_handlers.end() )
        return false;

    return (this->*(it->second))( control, reply );
}


bool lrs_device_controller::on_hardware_reset( nlohmann::json const & control, nlohmann::json & reply )
{
    _rs_dev.hardware_reset();
    return true;
}


