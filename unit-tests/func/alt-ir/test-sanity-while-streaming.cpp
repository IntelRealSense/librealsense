// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#test:device L500*

#include "../func-common.h"
#include "alt-ir-common.h"

using namespace rs2;

TEST_CASE( "AltIR", "[l500][live]" )
{
   
    auto devices = find_devices_by_product_line_or_exit( RS2_PRODUCT_LINE_L500 );
    auto dev = devices[0];
    
    auto depth_sens = dev.first< rs2::depth_sensor >();
    if( alt_ir_supported_or_message( depth_sens ) )
    {
        option_range r;
        REQUIRE_NOTHROW( r = depth_sens.get_option_range( RS2_OPTION_ALTERNATE_IR ) );

        start_default_l500_depth_profiles( depth_sens, [&]( rs2::frame f ) {} , true);

        for( auto i = r.min; i <= r.max; i+=r.step )
        {
            // We expect that no delay is needed between set and get, as would be the case
            // with XU options (during streaming)
            CHECK_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, i ) );
            CHECK( depth_sens.get_option( RS2_OPTION_ALTERNATE_IR ) == i );
        }
    
        depth_sens.stop();
        depth_sens.close();
    }
}
