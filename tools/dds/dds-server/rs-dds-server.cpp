// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/utilities/json.h>

#include <realdds/dds-device-broadcaster.h>
#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/dds-log-consumer.h>
#include <realdds/topics/device-info/device-info-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>

#include <fastrtps/types/TypesBase.h>
#include <fastdds/dds/log/Log.hpp>

#include "lrs-device-watcher.h"
#include "lrs-device-controller.h"

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>

#include <string>
#include <iostream>
#include <map>
#include <unordered_set>

using nlohmann::json;

using namespace TCLAP;
using namespace realdds;

//Pointer to the context created in main, for access from other functions
//Need to create a context only once or new context will create a dds_device_watcher and discover
//devices we published as new DDS devices
rs2::context * main_ctx = nullptr;

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


std::vector< std::shared_ptr< realdds::dds_stream_server > > get_supported_streams( rs2::device dev )
{
    std::map< std::string, realdds::dds_stream_profiles > name_to_profiles;
    std::map< std::string, int > name_to_default_profile;
    std::map< std::string, std::string > name_to_sensor;
    std::map< std::string, std::shared_ptr< realdds::dds_stream_server > > name_to_server;
    for( auto sensor : dev.query_sensors() )
    {
        std::string const sensor_name = sensor.get_info( RS2_CAMERA_INFO_NAME );
        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(), stream_profiles.end(), [&]( const rs2::stream_profile & sp ) {
            std::string stream_name = sp.stream_name();
            auto & server = name_to_server[stream_name];
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
            name_to_sensor[stream_name] = sensor_name;
            auto & profiles = name_to_profiles[stream_name];
            std::shared_ptr< realdds::dds_stream_profile > profile;
            if( sp.is< rs2::video_stream_profile >() )
            {
                auto const & vsp = sp.as< rs2::video_stream_profile >();
                profile = std::make_shared< realdds::dds_video_stream_profile >(
                    static_cast<int16_t>( vsp.fps() ),
                    realdds::dds_stream_format::from_rs2( vsp.format() ),
                    static_cast< uint16_t >( vsp.width() ),
                    static_cast< int16_t >( vsp.height() ) );
            }
            else if( sp.is< rs2::motion_stream_profile >() )
            {
                const auto & msp = sp.as< rs2::motion_stream_profile >();
                profile = std::make_shared< realdds::dds_motion_stream_profile >(
                    static_cast< int16_t >( msp.fps() ),
                    realdds::dds_stream_format::from_rs2( msp.format() ) );
            }
            else
            {
                LOG_ERROR( "unknown profile type of uid " << sp.unique_id() );
                return;
            }
            if( sp.is_default() )
                name_to_default_profile[stream_name] = static_cast< int >( profiles.size() );
            profiles.push_back( profile );
            LOG_DEBUG( stream_name << ": " << profile->to_string() );
        } );
    }
    std::vector< std::shared_ptr< realdds::dds_stream_server > > servers;
    for( auto & it : name_to_profiles )
    {
        auto const & stream_name = it.first;
        
        int default_profile_index = 0;
        auto default_profile_it = name_to_default_profile.find( stream_name );
        if( default_profile_it != name_to_default_profile.end() )
            default_profile_index = default_profile_it->second;

        auto const& profiles = it.second;
        if( profiles.empty() )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no profiles" );
            continue;
        }
        std::string const & sensor_name = name_to_sensor[stream_name];
        auto server = name_to_server[stream_name];
        if( ! server )
        {
            LOG_ERROR( "ignoring stream '" << stream_name << "' with no server" );
            continue;
        }
        server->init_profiles( profiles, default_profile_index );
        servers.push_back( server );
    }
    return servers;
}

#undef NAME2SERVER

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

rs2::stream_profile get_required_profile( rs2::sensor sensor,
    std::string stream_name,
    std::shared_ptr< dds_stream_profile > profile )
{
    auto sensor_stream_profiles = sensor.get_stream_profiles();
    auto profile_iter = std::find_if( sensor_stream_profiles.begin(),
        sensor_stream_profiles.end(),
        [&]( rs2::stream_profile sp ) {
            auto vp = sp.as< rs2::video_stream_profile >();
            auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( profile );
            bool video_params = true;
            if ( vp && dds_vp )
                video_params = vp.width() == dds_vp->width() && vp.height() == dds_vp->height();
            return sp.stream_type() == stream_name_to_type( stream_name )
                && sp.stream_index() == stream_name_to_index( stream_name )
                && sp.fps() == profile->frequency()
                && sp.format() == profile->format().to_rs2()
                && video_params;
        } );
    if ( profile_iter == sensor_stream_profiles.end() )
    {
        throw std::runtime_error( "Could not find required profile" );
    }

    return *profile_iter;
}

void open_streams_callback( const json & msg, dds_device_server * dds_dev_server )
{
    auto msg_profiles = msg["stream-profiles"];

    std::vector< rs2::stream_profile > rs_profiles_to_open;
    std::vector< std::pair < std::string, image_header > > realdds_streams_to_start;

    if ( !main_ctx )
        throw std::runtime_error( "No context available" );
    auto sensors = main_ctx->query_all_sensors();
    size_t sensor_index = 0;
    for ( auto it = msg_profiles.begin(); it != msg_profiles.end(); ++it )
    {
        //To get actual rs2::stream_profile to open we need to pass an rs2::sensor to `get_required_profile`.
        //For every profile to open we will iterate over all the sensors, if a sensor have a profile with a name
        //corresponding to the requested, we have found our sensor.
        bool found = false;
        std::string requested_stream_name = it.key();
        for ( ; sensor_index < sensors.size(); ++sensor_index )
        {
            for ( auto & profile : sensors[sensor_index].get_stream_profiles() )
            {
                if ( profile.stream_name() == requested_stream_name )
                {
                    found = true;
                    break;
                }
            }

            if ( found )
                break;
        }

        if ( sensor_index == sensors.size() )
            throw std::runtime_error( "Could not find sensor to open streams for" );

        //Now that we have the sensor get the rs2::stream_profile
        auto dds_profile = create_dds_stream_profile( stream_name_to_type( requested_stream_name ), it.value() );
        auto rs2_profile = get_required_profile( sensors[sensor_index],
                                                 requested_stream_name,
                                                 dds_profile );
        rs_profiles_to_open.push_back( rs2_profile );

        auto dds_vp = std::dynamic_pointer_cast< dds_video_stream_profile >( dds_profile );
        realdds::image_header header;
        header.format = dds_profile->format().to_rs2();
        header.width = dds_vp ? dds_vp->width() : 0;
        header.height = dds_vp ? dds_vp->height() : 0;
        realdds_streams_to_start.push_back( std::make_pair( requested_stream_name, std::move(header) ) );
    }

    //Start streaming
    //TODO - currently assumes all streams are from one sensor. Add support for multiple sensors
    dds_dev_server->start_streaming( realdds_streams_to_start );
    sensors[sensor_index].open( rs_profiles_to_open );
    sensors[sensor_index].start( [&]( rs2::frame f ) {
        dds_dev_server->publish_image( f.get_profile().stream_name(), static_cast< const uint8_t * >( f.get_data() ), f.get_data_size() );
    } );
    std::cout << realdds_streams_to_start[0].first << " stream started" << std::endl;
}


std::string get_topic_root( topics::device_info const & dev_info )
{
    // Build device root path (we use a device model only name like DXXX)
    // example: /realsense/D435/11223344
    constexpr char const * DEVICE_NAME_PREFIX = "Intel RealSense ";
    constexpr size_t DEVICE_NAME_PREFIX_CCH = 16;
    // We don't need the prefix in the path
    std::string model_name = dev_info.name;
    if ( model_name.length() > DEVICE_NAME_PREFIX_CCH
       && 0 == strncmp( model_name.data(), DEVICE_NAME_PREFIX, DEVICE_NAME_PREFIX_CCH ) )
    {
        model_name.erase( 0, DEVICE_NAME_PREFIX_CCH );
    }
    constexpr char const * RS_ROOT = "realsense/";
    return RS_ROOT + model_name + '/' + dev_info.serial;
}


topics::device_info rs2_device_to_info( rs2::device const & dev )
{
    topics::device_info dev_info;
    dev_info.name = dev.get_info( RS2_CAMERA_INFO_NAME );
    dev_info.serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    dev_info.product_line = dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
    dev_info.locked = ( dev.get_info( RS2_CAMERA_INFO_CAMERA_LOCKED ) == "YES" );

    // Build device topic root path
    dev_info.topic_root = get_topic_root( dev_info );
    return dev_info;
}


int main( int argc, char * argv[] )
try
{
    dds_domain_id domain = 0;
    CmdLine cmd( "librealsense rs-dds-server tool, use CTRL + C to stop..", ' ' );
    ValueArg< dds_domain_id > domain_arg( "d",
                                          "domain",
                                          "Select domain ID to listen on",
                                          false,
                                          0,
                                          "0-232" );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );

    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    // Configure the same logger as librealsense
    el::Configurations defaultConf;
    defaultConf.setToDefault();
    defaultConf.setGlobally( el::ConfigurationType::ToStandardOutput, debug_arg.isSet() ? "true" : "false" );
    if( ! debug_arg.isSet() )
        defaultConf.set( el::Level::Error, el::ConfigurationType::ToStandardOutput, "true" );
    defaultConf.setGlobally( el::ConfigurationType::Format, "-%levshort- %datetime{%H:%m:%s.%g} %msg (%fbase:%line [%thread])" );
    el::Loggers::reconfigureLogger( "librealsense", defaultConf );

    // Intercept DDS messages and redirect them to our own logging mechanism
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( realdds::log_consumer::create() );

    if( debug_arg.isSet() )
    {
        rs2::log_to_console( RS2_LOG_SEVERITY_DEBUG );
        eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Info );
    }
    else
    {
        rs2::log_to_console( RS2_LOG_SEVERITY_ERROR );
        eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Error );
    }

    if( domain_arg.isSet() )
    {
        domain = domain_arg.getValue();
        if( domain > 232 )
        {
            std::cerr << "Invalid domain value, enter a value in the range [0, 232]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    std::cout << "Starting RS DDS Server.." << std::endl;

    // Create a DDS publisher
    auto participant = std::make_shared< dds_participant >();
    participant->init( domain, "rs-dds-server" );

    // Run the DDS device broadcaster
    dds_device_broadcaster broadcaster( participant );
    if( !broadcaster.run() )
    {
        std::cerr << "Failure running the DDS Device Broadcaster" << std::endl;
        return EXIT_FAILURE;
    }

    struct device_handler
    {
        topics::device_info info;
        std::shared_ptr< dds_device_server > server;
        std::shared_ptr< tools::lrs_device_controller > controller;
    };
    std::map< rs2::device, device_handler > device_handlers_list;
    
    std::cout << "Start listening to RS devices.." << std::endl;

    // Create a RealSense context
    rs2::context ctx( "{"
        "\"dds-discovery\" : false"
        "}" );
    main_ctx = &ctx;

    // Run the LRS device watcher
    tools::lrs_device_watcher dev_watcher( ctx );
    dev_watcher.run(
        // Handle a device connection
        [&]( rs2::device dev ) {

            auto dev_info = rs2_device_to_info( dev );

            // Broadcast the new connected device to all listeners
            broadcaster.add_device( dev_info );

            // Create a dds-device-server for this device
            auto dds_device_server
                = std::make_shared< realdds::dds_device_server >( participant, dev_info.topic_root );
            dds_device_server->on_open_streams( open_streams_callback );

            // Create a supported streams list for initializing the relevant DDS topics
            auto supported_streams = get_supported_streams( dev );

            // Initialize the DDS device server with the supported streams
            dds_device_server->init( supported_streams );

            // Create a lrs_device_manager for this device
            std::shared_ptr< tools::lrs_device_controller > lrs_device_controller
                = std::make_shared< tools::lrs_device_controller >( dev );

            // Keep a pair of device controller and server per RS device
            device_handlers_list.emplace( dev, device_handler{ dev_info, dds_device_server, lrs_device_controller } );
        },
        // Handle a device disconnection
        [&]( rs2::device dev ) {
            // Remove the dds-server for this device
            auto const & handler = device_handlers_list.at( dev );

            // Remove this device from the DDS device broadcaster
            broadcaster.remove_device( handler.info );

            handler.controller->stop_all_streams();
            device_handlers_list.erase( dev );
        } );

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), 0);// Pend until CTRL + C is pressed 

    std::cout << "Shutting down rs-dds-server..." << std::endl;

    return EXIT_SUCCESS;
}
catch( const rs2::error & e )
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args()
              << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch( const std::exception & e )
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}
