// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include <unit-tests/test.h>
#include <src/concurrency.h>

using namespace rs2;


bool alt_ir_supported_or_message( const rs2::depth_sensor & depth_sens )
{
    REQUIRE( depth_sens );

    if( depth_sens.supports( RS2_OPTION_ALTERNATE_IR ) )
        return true;

    std::cout << "FW version " << depth_sens.get_info( RS2_CAMERA_INFO_FIRMWARE_VERSION )
                << " doesn't support alt IR option";
    return false;
}

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

    auto calib = dev.as< rs2::device_calibration >();
    if( calib )
    {
        rs2_calibration_status status;
        calib.register_calibration_change_callback( [&]( rs2_calibration_status cal_status ) {
            status = cal_status;
        } );

        // Before throwing, AC will notify of a BAD_CONDITIONS status
        REQUIRE_THROWS( calib.trigger_device_calibration( RS2_CALIBRATION_MANUAL_DEPTH_TO_RGB ) );

        REQUIRE( status == RS2_CALIBRATION_BAD_CONDITIONS );
    }

    REQUIRE_NOTHROW( depth_sens.stop() );
    REQUIRE_NOTHROW( depth_sens.close() );
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

    REQUIRE_NOTHROW( depth_sens.stop() );
    REQUIRE_NOTHROW( depth_sens.close() );

}
