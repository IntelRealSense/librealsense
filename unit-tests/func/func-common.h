// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../test.h"
#include "librealsense2/rs.hpp"
#include <condition_variable>

using namespace rs2;

inline rs2::device_list find_devices_by_product_line_or_exit( int product )
{
    rs2::context ctx;
    rs2::device_list devices_list = ctx.query_devices( product );
    if( devices_list.size() == 0 )
    {
        std::cout << "No device of the " << product << " product line was found; skipping test"
                  << std::endl;
        exit( 0 );
    }

    return devices_list;
}

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

// Remove the frame's stream (or streams if a frameset) from the list of streams we expect to arrive
// If any stream is unexpected, it is ignored
inline void remove_all_streams_arrived( rs2::frame f,
                                        std::vector< rs2::stream_profile > & expected_streams )
{
    auto remove_stream = [&]() {
        auto it = std::remove_if( expected_streams.begin(),
                                  expected_streams.end(),
                                  [&]( rs2::stream_profile s ) {
                                      return s.stream_type() == f.get_profile().stream_type();
                                  } );


        if( it != expected_streams.end() )
            expected_streams.erase( it );
    };

    if( f.is< rs2::frameset >() )
    {
        auto set = f.as< rs2::frameset >();
        set.foreach_rs( [&]( rs2::frame fr ) { remove_stream(); } );
    }
    else
    {
        remove_stream();
    }
}

inline stream_profile find_default_depth_profile( rs2::depth_sensor depth_sens )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto depth_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_DEPTH;
          } );

    REQUIRE( depth_profile != stream_profiles.end() );
    return *depth_profile;
}

inline stream_profile find_default_ir_profile( rs2::depth_sensor depth_sens )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto ir_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_INFRARED;
          } );

    REQUIRE( ir_profile != stream_profiles.end() );
    return *ir_profile;
}

inline stream_profile find_confidence_corresponding_to_depth( rs2::depth_sensor depth_sens,
                                                       stream_profile depth_profile )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto confidence_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), [&]( stream_profile sp ) {
              return sp.stream_type() == RS2_STREAM_CONFIDENCE
                  && sp.as< rs2::video_stream_profile >().width()
                         == depth_profile.as< rs2::video_stream_profile >().width()
                  && sp.as< rs2::video_stream_profile >().height()
                         == depth_profile.as< rs2::video_stream_profile >().height();
          } );

    REQUIRE( confidence_profile != stream_profiles.end() );
    return *confidence_profile;
}
