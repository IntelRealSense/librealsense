// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <concurrency.h>

// This should be defined, per device type
void trigger_error_or_exit( const rs2::device & dev, uint8_t num );

// Loop over all possible hw errors and trigger them. Check that the user callback gets the error.
void validate_errors_handling( const rs2::device & dev,
                               const std::map< uint8_t, std::pair< std::string, rs2_log_severity > > & error_report )
{
    auto depth_sens = dev.first< rs2::depth_sensor >();

    REQUIRE( depth_sens.supports( RS2_OPTION_ERROR_POLLING_ENABLED ) );

    std::string notification_description;
    rs2_log_severity severity;
    std::condition_variable cv;
    std::mutex m;

    depth_sens.set_notifications_callback( [&]( rs2::notification n ) {
        std::lock_guard< std::mutex > lock( m );

        notification_description = n.get_description();
        severity = n.get_severity();
        cv.notify_one();
    } );

    REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ERROR_POLLING_ENABLED, 0.1f ) );

    for( auto i = error_report.begin(); i != error_report.end(); i++ )
    {
        trigger_error_or_exit( dev, i->first );
        std::unique_lock< std::mutex > lock( m );
        CAPTURE( i->first );

        auto pred = [&]() {
            return notification_description.compare( i->second.first ) == 0
                && severity == i->second.second;
        };
        REQUIRE( cv.wait_for( lock, std::chrono::seconds( 5 ), pred ) );
    }

    depth_sens.set_option(RS2_OPTION_ERROR_POLLING_ENABLED, 0);
}
