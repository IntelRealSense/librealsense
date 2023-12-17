// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-device-server.h>
#include <realdds/dds-stream-server.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-option.h>
#include <realdds/dds-log-consumer.h>
#include <realdds/topics/device-info-msg.h>

#include "lrs-device-watcher.h"
#include "lrs-device-controller.h"

#include <tclap/CmdLine.h>
#include <tclap/ValueArg.h>

#include <rsutils/os/executable-name.h>
#include <rsutils/os/special-folder.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/json.h>

#include <string>
#include <iostream>
#include <map>
#include <set>

using namespace TCLAP;
using namespace realdds;


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
    return RS_ROOT + model_name + '_' + dev_info.serial;
}


topics::device_info rs2_device_to_info( rs2::device const & dev )
{
    topics::device_info dev_info;
    dev_info.name = dev.get_info( RS2_CAMERA_INFO_NAME );
    dev_info.serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
    dev_info.product_line = dev.get_info( RS2_CAMERA_INFO_PRODUCT_LINE );
    dev_info.locked = ( strcmp( dev.get_info( RS2_CAMERA_INFO_CAMERA_LOCKED ), "YES" ) == 0 );

    // Build device topic root path
    dev_info.topic_root = get_topic_root( dev_info );
    return dev_info;
}


static nlohmann::json load_settings( nlohmann::json const & local_settings )
{
    nlohmann::json config;

    // Load the realsense configuration file settings
    std::ifstream f( rsutils::os::get_special_folder( rsutils::os::special_folder::app_data ) + RS2_CONFIG_FILENAME );
    if( f.good() )
    {
        try
        {
            config = nlohmann::json::parse( f );
        }
        catch( std::exception const & e )
        {
            throw std::runtime_error( "failed to load configuration file: " + std::string( e.what() ) );
        }
    }

    config = rsutils::json::load_settings( config, "context", "config-file" );

    // Take the "dds" settings only
    config = rsutils::json::nested( config, "dds" );

    // We should always have DDS enabled
    if( config.is_object() )
        config.erase( "enabled" );

    // Patch the given local settings into the configuration
    rsutils::json::patch( config, local_settings, "local settings" );

    return config;
}


int main( int argc, char * argv[] )
try
{
    dds_domain_id domain = -1;  // from settings; default to 0
    CmdLine cmd( "librealsense rs-dds-adapter tool, use CTRL + C to stop..", ' ' );
    ValueArg< dds_domain_id > domain_arg( "d",
                                          "domain",
                                          "Select domain ID to publish on",
                                          false,
                                          0,
                                          "0-232" );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );

    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    // Configure the same logger as librealsense
    rsutils::configure_elpp_logger( debug_arg.isSet() );
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

    std::cout << "Starting RS DDS Adapter.." << std::endl;

    // Create a DDS participant
    auto participant = std::make_shared< dds_participant >();
    {
        nlohmann::json dds_settings = nlohmann::json::object();
        dds_settings = load_settings( dds_settings );
        participant->init( domain, "rs-dds-adapter", std::move( dds_settings ) );
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
    nlohmann::json j = {
        { "dds",               false   }, // Don't discover DDS devices from the network, we want local devices only 
        { "format-conversion", "basic" }  // Don't convert raw sensor formats (except interleaved) will be done by receiver
    };
    rs2::context ctx( j.dump() );

    // Run the LRS device watcher
    tools::lrs_device_watcher dev_watcher( ctx );
    dev_watcher.run(
        // Handle a device connection
        [&]( rs2::device dev ) {

            auto dev_info = rs2_device_to_info( dev );

            // Create a dds-device-server for this device
            auto dds_device_server
                = std::make_shared< realdds::dds_device_server >( participant, dev_info.topic_root );
 
            // Create a lrs_device_manager for this device
            std::shared_ptr< tools::lrs_device_controller > lrs_device_controller
                = std::make_shared< tools::lrs_device_controller >( dev, dds_device_server );

            // Finally, we're ready to broadcast it
            dds_device_server->broadcast( dev_info );

            // Keep a pair of device controller and server per RS device
            device_handlers_list.emplace( dev, device_handler{ dev_info, dds_device_server, lrs_device_controller } );
        },
        // Handle a device disconnection
        [&]( rs2::device dev ) {
            // Remove the dds-server for this device
            auto const & handler = device_handlers_list.at( dev );

            device_handlers_list.erase( dev );
        } );

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), 0);// Pend until CTRL + C is pressed 

    std::cout << "Shutting down rs-dds-adapter..." << std::endl;

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
