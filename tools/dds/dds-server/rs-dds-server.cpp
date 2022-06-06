// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <map>
#include <unordered_set>

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/dds/dds-device-broadcaster.h>
#include <librealsense2/dds/dds-device-server.h>
#include <librealsense2/dds/dds-participant.h>
#include <fastrtps/types/TypesBase.h>
#include <fastdds/dds/log/Log.hpp>

#include "lrs-device-watcher.h"
#include "lrs-device-controller.h"

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

using namespace TCLAP;

std::vector< std::string > get_supported_streams( rs2::device dev )
{
    auto device_sensors = dev.query_sensors();
    std::unordered_set< std::string > supported_streams_names;
    for( auto sensor : device_sensors )
    {
        auto stream_profiles = sensor.get_stream_profiles();
        std::for_each( stream_profiles.begin(),
                       stream_profiles.end(),
                       [&]( const rs2::stream_profile & sp ) {
                           supported_streams_names.insert( sp.stream_name() );
                       } );
    }

    return std::vector< std::string >( supported_streams_names.begin(), supported_streams_names.end() );
}


rs2::stream_profile get_required_profile( rs2::sensor sensor,
                                          const rs2_stream stream,
                                          const int fps,
                                          rs2_format const format,
                                          const int width,
                                          const int height )
{
    auto sensor_stream_profiles = sensor.get_stream_profiles();
    auto found_profiles = std::find_if( sensor_stream_profiles.begin(),
                                        sensor_stream_profiles.end(),
                                        [&]( rs2::stream_profile sp ) {
                                            auto vp = sp.as< rs2::video_stream_profile >();
                                            return sp.stream_type() == stream && sp.fps() == fps
                                                && sp.format() == format && vp.width() == width
                                                && vp.height() == height;
                                        } );
    if( found_profiles == sensor_stream_profiles.end() )
    {
        throw std::runtime_error( "Could not find required profile" );
    }

    return *found_profiles;
}

void start_streaming( std::shared_ptr< tools::lrs_device_controller > lrs_device_controller,
                      std::shared_ptr< librealsense::dds::dds_device_server > dds_dev_server,
                      const rs2::stream_profile & stream_profile )
{
    // Configure DDS-server to the required frame header
    librealsense::dds::dds_device_server::image_header header;
    auto vsp = stream_profile.as< rs2::video_stream_profile >();
    header.format = vsp.format();
    header.height = vsp.height();
    header.width = vsp.width();
    dds_dev_server->set_image_header( stream_profile.stream_name(), header );

    // Start streaming
    lrs_device_controller->start_stream( stream_profile, [&, dds_dev_server]( rs2::frame f ) {
        auto vf = f.as< rs2::video_frame >();
        try
        {
            dds_dev_server->publish_image( vf.get_profile().stream_name(),
                                           (const uint8_t *)f.get_data(),
                                           f.get_data_size() );
        }
        catch( std::exception & e )
        {
            LOG_ERROR( "Exception raised during DDS publish " << vf.get_profile().stream_name()
                                                              << " frame: " << e.what() );
        }
    } );
}


struct log_consumer : eprosima::fastdds::dds::LogConsumer
{
    virtual void Consume( const eprosima::fastdds::dds::Log::Entry & e ) override
    {
        using eprosima::fastdds::dds::Log;
        switch( e.kind )
        {
        case Log::Kind::Error:
            LOG_ERROR( "[DDS] " << e.message );
            break;
        case Log::Kind::Warning:
            LOG_WARNING( "[DDS] " << e.message );
            break;
        case Log::Kind::Info:
            LOG_DEBUG( "[DDS] " << e.message );
            break;
        }
    }
};


int main( int argc, char * argv[] )
try
{
    librealsense::dds::dds_domain_id domain = 0;
    CmdLine cmd( "librealsense rs-dds-server tool, use CTRL + C to stop..", ' ' );
    ValueArg< librealsense::dds::dds_domain_id > domain_arg( "d",
                                                             "domain",
                                                             "Select domain ID to listen on",
                                                             false,
                                                             0,
                                                             "0-232" );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );

    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    // Intercept DDS messages and redirect them to our own logging mechanism
    std::unique_ptr< eprosima::fastdds::dds::LogConsumer > consumer( new log_consumer() );
    eprosima::fastdds::dds::Log::ClearConsumers();
    eprosima::fastdds::dds::Log::RegisterConsumer( std::move( consumer ) );

    if( debug_arg.isSet() )
    {
        rs2::log_to_console( RS2_LOG_SEVERITY_DEBUG );
        eprosima::fastdds::dds::Log::SetVerbosity( eprosima::fastdds::dds::Log::Info );
    }
    else
    {
        rs2::log_to_console( RS2_LOG_SEVERITY_ERROR );
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
    librealsense::dds::dds_participant participant;
    participant.init( domain, "rs-dds-server" );

    // Run the DDS device broadcaster
    librealsense::dds::dds_device_broadcaster broadcaster( participant );
    if( !broadcaster.run() )
    {
        std::cerr << "Failure running the DDS Device Broadcaster" << std::endl;
        return EXIT_FAILURE;
    }

    std::map<rs2::device, std::pair<std::shared_ptr<librealsense::dds::dds_device_server>, std::shared_ptr<tools::lrs_device_controller>>> device_handlers_list;
    
    std::cout << "Start listening to RS devices.." << std::endl;

    // Create a RealSense context
    rs2::context ctx( "{"
        "\"dds-discovery\" : false"
        "}" );

    // Run the LRS device watcher
    tools::lrs_device_watcher dev_watcher( ctx );
    dev_watcher.run(
        // Handle a device connection
        [&]( rs2::device dev ) {

            // Broadcast the new connected device to all listeners
            auto dev_topic_root = broadcaster.add_device( dev );

            // Create a supported streams list for initializing the relevant DDS topics
            std::vector<std::string> supported_streams_names_vec = get_supported_streams( dev );

            // Create a dds-device-server for this device
            std::shared_ptr< librealsense::dds::dds_device_server > dds_device_server
                = std::make_shared< librealsense::dds::dds_device_server >( participant,
                                                                            dev_topic_root );
            // Initialize the DDS device server with the supported streams
            dds_device_server->init( supported_streams_names_vec );

            // Create a lrs_device_manager for this device
            std::shared_ptr< tools::lrs_device_controller > lrs_device_controller
                = std::make_shared< tools::lrs_device_controller >( dev );

            // Keep a pair of device controller and server per RS device
            device_handlers_list.insert(
                { dev, { dds_device_server, lrs_device_controller } } );

            // Get the desired stream profile
            auto profile = get_required_profile( dev.first< rs2::color_sensor >(),
                                  RS2_STREAM_COLOR,
                                  30,
                                  RS2_FORMAT_RGB8,
                                  1280,
                                  720 );

            // Start streaming 
            start_streaming( lrs_device_controller, dds_device_server, profile );
        },
        // Handle a device disconnection
        [&]( rs2::device dev ) {
            // Remove the dds-server for this device
            auto & lrs_dev_manager = device_handlers_list.at( dev ).second;

            lrs_dev_manager->stop_all_streams();
            device_handlers_list.erase( dev );

            // Remove this device from the DDS device broadcaster
            broadcaster.remove_device( dev );
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
