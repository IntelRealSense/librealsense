// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

// See RSDSO-19304 or Github #10482 for background

//#test:device D400*

// The deadlock causes this test to time out but we don't want to wait the full 200 seconds
//#test:timeout 12

//#test:donotrun:!nightly

#include "live-common.h"
#include <chrono>

using namespace rs2;


TEST_CASE( "deadlock after hw reset", "[live]" )
{
    rs2::context ctx( "{\"dds\":0}" );

    rs2::device_list list = ctx.query_devices();
    test::log.i( list.size(), "RealSense devices connected" );

    test::log.i( "Resetting" );
    for( uint32_t dev_idx = 0; dev_idx < list.size(); dev_idx++ )
        list[dev_idx].hardware_reset();

    test::log.i( "Sleeping 6 seconds" );
    std::this_thread::sleep_for( std::chrono::seconds( 6 ) );

    list = ctx.query_devices();
    test::log.i( list.size(), "devices after reset" );

    test::log.i( "This caused deadlock" );
    for( uint32_t dev_idx = 0; dev_idx < list.size(); dev_idx++ )
    {
        rs2::device dev;
        dev = list[dev_idx];

        // Add 500 ms delay to avoid deadlock
        //std::this_thread::sleep_for( std::chrono::milliseconds( 500 ));
    }
}
