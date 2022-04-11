// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>
#include "dds-server.h"

#include <fastrtps/types/TypesBase.h>
#include "tclap/CmdLine.h"
#include "tclap/ValueArg.h"

using namespace TCLAP;

int main( int argc, char * argv[] )
try
{
    eprosima::fastdds::dds::DomainId_t domain = 0;
    CmdLine cmd( "librealsense rs-dds-server tool, use CTRL + C to stop..", ' ' );
    ValueArg< eprosima::fastdds::dds::DomainId_t > domain_arg( "d",
                                                               "domain",
                                                               "Select domain ID to listen on",
                                                               false,
                                                               0,
                                                               "0-232" );
    SwitchArg debug_arg( "", "debug", "Enable debug logging", false );

    cmd.add( domain_arg );
    cmd.add( debug_arg );
    cmd.parse( argc, argv );

    if( debug_arg.isSet() )
        rs2::log_to_console( RS2_LOG_SEVERITY_DEBUG );
    if( domain_arg.isSet() )
    {
        domain = domain_arg.getValue();
        if( domain > 232 )
        {
            std::cerr << "Invalid domain value, enter a value in the range [0, 232]" << std::endl;
            return EXIT_FAILURE;
        }
    }

    dds_server my_dds_server;
    if( my_dds_server.init( domain ) )
    {
        my_dds_server.run( );
    }
    else
    {
        std::cerr << "Initialization failure" << std::endl;
        return EXIT_FAILURE;
    }

    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), 0);// Pend until CTRL + C is pressed 

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
