// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <concurrency.h>

using namespace rs2;

void set_alt_ir_if_needed( const rs2::depth_sensor & depth_sens, float val )
{
    REQUIRE( depth_sens.supports( RS2_OPTION_ALTERNATE_IR ) );
    auto alt_ir = depth_sens.get_option( RS2_OPTION_ALTERNATE_IR );
    if( alt_ir != val )
        REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ALTERNATE_IR, val ) );
}

void enable_alt_ir_and_check_that_AC_fails(
    const rs2::device & dev,
    const rs2::depth_sensor & depth_sens,
    const std::vector< stream_profile > & expected_profiles )
{
    std::vector< stream_profile > profiles = expected_profiles;
    REQUIRE_NOTHROW( depth_sens.open( profiles ) );

    REQUIRE_NOTHROW( depth_sens.start( [&]( rs2::frame f ) {} ) );

    set_alt_ir_if_needed( depth_sens, 1 );

    // check that AC throws exception
    if( dev.is< rs2::device_calibration >() )
    {
        std::condition_variable cv;
        std::mutex m;

        auto calib = dev.as< rs2::device_calibration >();

        rs2_calibration_status status;
        calib.register_calibration_change_callback( [&]( rs2_calibration_status cal_status ) {
            status = cal_status;
        } );

        REQUIRE_THROWS( calib.trigger_device_calibration( RS2_CALIBRATION_MANUAL_DEPTH_TO_RGB ) );

        REQUIRE( status == RS2_CALIBRATION_BAD_CONDITIONS );
    }

    depth_sens.stop();
    depth_sens.close();
}

void enable_alt_ir_and_check_that_all_streams_arrived(
    const rs2::device & dev,
    const rs2::depth_sensor & depth_sens,
    const std::vector< stream_profile > & expected_profiles )
{
    std::vector< stream_profile > profiles = expected_profiles;
    REQUIRE_NOTHROW( depth_sens.open( profiles ) );

    std::condition_variable cv;
    std::mutex m;

    auto wait_for_streams = [&]() {
        profiles = expected_profiles;
        std::unique_lock< std::mutex > lock( m );
        REQUIRE( cv.wait_for( lock, std::chrono::seconds( 20 ), [&]() {
            return profiles.size() == 0;
        } ) );
    };

    REQUIRE_NOTHROW( depth_sens.start( [&]( rs2::frame f ) {
        std::unique_lock< std::mutex > lock( m );
        remove_all_streams_arrived( f, profiles );
        cv.notify_one();
    } ) );

    set_alt_ir_if_needed( depth_sens, 1 );
    wait_for_streams();

    set_alt_ir_if_needed( depth_sens, 0 );
    wait_for_streams();

    set_alt_ir_if_needed( depth_sens, 1 );
    wait_for_streams();

    depth_sens.stop();
    depth_sens.close();

}