// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../test.h"
#include "librealsense2/rs.hpp"
#include <condition_variable>
#include "hw-monitor.h"

using namespace rs2;

inline rs2::device reset_camera_and_wait_for_connection( rs2::device & dev)
{
    rs2::context ctx;
    std::mutex m;
    bool disconnected = false;
    bool connected = false;
    rs2::device result;
    std::condition_variable cv;

    std::string serial = dev.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );

    ctx.set_devices_changed_callback( [&] (rs2::event_information info ) mutable {
        if( info.was_removed( dev ) )
        {
            std::unique_lock< std::mutex > lock( m );
            disconnected = true;
            cv.notify_all();
        }
        auto list = info.get_new_devices();
        if( list.size() > 0 )
        {
            for( auto cam : list )
            {
                auto new_ser = cam.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER );
                if( serial == cam.get_info( RS2_CAMERA_INFO_SERIAL_NUMBER ) )
                {
                    std::unique_lock< std::mutex > lock( m );
                    connected = true;
                    result = cam ;

                    cv.notify_all();
                    break;
                }
            }
        }
    } );

    dev.hardware_reset();

    std::unique_lock< std::mutex > lock( m );
    REQUIRE(cv.wait_for( lock, std::chrono::seconds( 50 ), [&]() { return disconnected; } ) );
    REQUIRE( cv.wait_for( lock, std::chrono::seconds( 50 ), [&]() { return connected; } ) );
    REQUIRE( result );
    return result;
}
