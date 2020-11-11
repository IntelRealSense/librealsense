// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../tests.h"
#include "../include/librealsense2/rs.hpp"
#include "../include/librealsense2/hpp/rs_context.hpp"
#include "../include/librealsense2/hpp/rs_internal.hpp"
#include "concurrency.h"

using namespace rs2;

std::vector< stream_profile > find_profiles_to_stream( rs2::depth_sensor depth_sens )
{
    std::vector< stream_profile > stream_profiles;
    REQUIRE_NOTHROW( stream_profiles = depth_sens.get_stream_profiles() );

    auto depth_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_DEPTH;
          } );

    auto ir_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), []( stream_profile sp ) {
              return sp.is_default() && sp.stream_type() == RS2_STREAM_INFRARED;
          } );

    auto confidence_profile
        = std::find_if( stream_profiles.begin(), stream_profiles.end(), [&]( stream_profile sp ) {
              return sp.stream_type() == RS2_STREAM_CONFIDENCE
                  && sp.as< rs2::video_stream_profile >().width()
                         == ir_profile->as< rs2::video_stream_profile >().width()
                  && sp.as< rs2::video_stream_profile >().height()
                         == ir_profile->as< rs2::video_stream_profile >().height();
          } );

    return { *depth_profile, *ir_profile, *confidence_profile };
}

void enable_alt_ir_and_check_that_AC_fails( const rs2::device& dev,
                          const rs2::depth_sensor& depth_sens,
                          const std::vector< stream_profile > & expected_profiles )
{
    std::vector< stream_profile > profiles = expected_profiles;
    REQUIRE_NOTHROW( depth_sens.open( profiles ) );

    std::condition_variable cv;
    std::mutex m;

    // wait for all streams
    // scope for unique_lock
    {
        REQUIRE_NOTHROW( depth_sens.start( [&]( rs2::frame f ) {
            std::unique_lock< std::mutex > lock( m );
            remove_all_streams_arrived( f, profiles );
            cv.notify_one();
        } ) );

        std::unique_lock< std::mutex > lock( m );
        REQUIRE( cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() {
            return profiles.size() == 0;
        } ) );
    }

    // set alt_ir if it was disabled 
    REQUIRE( depth_sens.supports( RS2_OPTION_ALTERNATE_IR ) );
    auto alt_ir = depth_sens.get_option( RS2_OPTION_ALTERNATE_IR );
    if( alt_ir == 0)
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, 1 ) );

    // check that AC throws exception
    if( dev.is< rs2::device_calibration >() )
    {
        auto calib = dev.as< rs2::device_calibration >();

        rs2_calibration_status status;
        calib.register_calibration_change_callback( [&]( rs2_calibration_status cal_status ) {
            std::unique_lock< std::mutex > lock( m );
            status = cal_status;
            cv.notify_one();
        } );

        REQUIRE_THROWS( calib.trigger_device_calibration( RS2_CALIBRATION_MANUAL_DEPTH_TO_RGB ) );

        std::unique_lock< std::mutex > lock( m );
        REQUIRE( cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() {
            return status == RS2_CALIBRATION_BAD_CONDITIONS;
        } ) );
    }

    depth_sens.stop();
    depth_sens.close();

    // return alt IR to default
    option_range r;
    REQUIRE_NOTHROW( r = depth_sens.get_option_range( RS2_OPTION_ALTERNATE_IR ) );
    REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, r.def ) );
}
