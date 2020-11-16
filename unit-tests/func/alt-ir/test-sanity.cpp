// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include "../func-common.h"

using namespace rs2;

TEST_CASE( "AltIR", "[l500][live]" )
{
   
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];
    
    std::string serial;
    REQUIRE_NOTHROW( serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) );
    
    auto depth_sens = dev.first< rs2::depth_sensor >();
    
    if( depth_sens.supports(RS2_OPTION_ALTERNATE_IR) )
    {
        option_range r;
        REQUIRE_NOTHROW( r = depth_sens.get_option_range( RS2_OPTION_ALTERNATE_IR ) );
        REQUIRE( depth_sens.get_option( RS2_OPTION_ALTERNATE_IR ) == r.def );
    
        for( auto i = r.min; i <= r.max; i++ )
        {
            REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, i ) );
            REQUIRE( depth_sens.get_option( RS2_OPTION_ALTERNATE_IR ) == i );
        }
    
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, r.def ) );
    }
    else
        std::cout << depth_sens.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION )
                  << " doesn't support alt IR option";
}
