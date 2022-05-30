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
#include "lrs-device-manager.h"

#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

using namespace TCLAP;

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

    std::map<rs2::device, std::pair<std::shared_ptr<librealsense::dds::dds_device_server>, std::shared_ptr<tools::lrs_device_manager>>> device_handlers_list;
    
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
            auto device_sensors = dev.query_sensors();
            std::unordered_set<std::string> supported_streams_names;
            for( auto sensor : device_sensors )
            {
                auto stream_profiles = sensor.get_stream_profiles();
                std::for_each( stream_profiles.begin(),
                               stream_profiles.end(),
                               [&]( const rs2::stream_profile & sp ) {
                                   if( supported_streams_names.find( sp.stream_name() )
                                       == supported_streams_names.end() )
                                   {
                                       supported_streams_names.insert( sp.stream_name() );
                                   }
                               } );
            }

            // Create a dds-device-server for this device
            std::vector<std::string> supported_streams_names_vec( supported_streams_names.begin(), supported_streams_names.end() );
            std::shared_ptr< librealsense::dds::dds_device_server > new_dds_device_server
                = std::make_shared< librealsense::dds::dds_device_server >( participant,
                                                                            dev_topic_root );
            if( !new_dds_device_server->init( supported_streams_names_vec ) )
            {
                std::cerr << "Failure initializing dds_device_server for topic root: " << dev_topic_root << std::endl;
                return;
            }

            // Create a lrs_device_manager for this device
            std::shared_ptr< tools::lrs_device_manager > new_lrs_device_manager
                = std::make_shared< tools::lrs_device_manager >( dev );


            device_handlers_list.insert(
                { dev, { new_dds_device_server, new_lrs_device_manager } } );

            // Start RGB streaming
            new_lrs_device_manager->start_stream(
                tools::stream_type::RGB,
                [&, dev]( const std::string & stream_name, uint8_t * frame ) {
                    auto &dev_handler = device_handlers_list.at( dev );
                    auto& dds_device_server = dev_handler.first;
                    try
                    {
                        dds_device_server->publish_frame( "Color", frame );
                    }
                    catch( std::exception &e )
                    {
                        LOG_ERROR( "Exception raised during DDS publish RGB frame: " << e.what() );
                    }
                    
                } );
        },
        // Handle a device disconnection
        [&]( rs2::device dev ) {
            // Remove the dds-server for this device
            auto & dev_handler = device_handlers_list.at( dev );
            dev_handler.second->stop_stream( tools::stream_type::RGB );
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
