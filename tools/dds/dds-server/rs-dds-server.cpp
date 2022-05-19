// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <librealsense2/dds/dds-device-broadcaster.h>
#include <librealsense2/dds/dds-participant.h>
#include <fastrtps/types/TypesBase.h>
#include <fastdds/dds/log/Log.hpp>

#include "lrs-device-watcher.h"
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

    
    std::cout << "Start listening to RS devices.." << std::endl;
    // Run the LRS device watcher
    tools::lrs_device_watcher dev_watcher;
    dev_watcher.run( [&]( rs2::device dev ) { broadcaster.add_device( dev ); },
                     [&]( rs2::device dev ) { broadcaster.remove_device( dev ); } );

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), 0);// Pend until CTRL + C is pressed 

    std::cout << "Shutting down RS DDS Server..." << std::endl;

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
