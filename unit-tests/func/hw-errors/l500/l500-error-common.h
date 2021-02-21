// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

#include "../../../test.h"
#include <concurrency.h>

using namespace rs2;

const uint8_t l500_trigger_error_opcode = 0x5E;

// Possible error values
const std::map< uint8_t, std::string > l500_error_report
    = { { 1, "Fatal error occur and device is unable \nto run depth stream" },
        { 2, "Overflow occur on infrared stream" },
        { 3, "Overflow occur on depth stream" },
        { 4, "Overflow occur on confidence stream" },
        { 5, "Receiver light saturation, stream stopped for 1 sec" },
        { 6, "Error that may be overcome in few sec. \nStream stoped. May be recoverable" },
        { 7, "Warning, temperature close to critical" },
        { 8, "Critical temperature reached" },
        { 9, "DFU error" },
        { 12, "Fall detected stream stopped" },
        { 14, "Fatal error occurred (14)" },
        { 15, "Fatal error occurred (15)" },
        { 16, "Fatal error occurred (16)" },
        { 17, "Fatal error occurred (17)" },
        { 18, "Fatal error occurred (18)" },
        { 19, "Fatal error occurred (19)" },
        { 20, "Fatal error occurred (20)" },
        { 21, "Fatal error occurred (21)" },
        { 22, "Fatal error occurred (22)" },
        { 23, "Fatal error occurred (23)" },
        { 24, "Fatal error occurred (24)" } };

void trigger_error_or_exit( const rs2::device & dev, uint8_t num )
{
    std::vector< uint8_t > raw_data( 24, 0 );
    raw_data[0] = 0x14;
    raw_data[2] = 0xab;
    raw_data[3] = 0xcd;
    raw_data[4] = l500_trigger_error_opcode;
    raw_data[12] = num;

    if( auto debug = dev.as< debug_protocol >() )
    {
        try
        {
            debug.send_and_receive_raw_data( raw_data );
        }
        catch( ... )
        {
            std::cout << "This device doesn't support trigger_error" << std::endl;
            exit( 0 );
        }
    }
}

// This function loop over all posiable hw errors use hw-monitor command to trigger them
// and check that we call to user callback with the error
void validate_errors_handling( const rs2::device & dev,
                               const std::map< uint8_t, std::string > & error_report )
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

    REQUIRE_NOTHROW( depth_sens.set_option( RS2_OPTION_ERROR_POLLING_ENABLED, 1 ) );

    for( auto i = error_report.begin(); i != error_report.end(); i++ )
    {
        trigger_error_or_exit( dev, i->first );
        std::unique_lock< std::mutex > lock( m );
        CAPTURE( i->first );

        auto pred = [&]() {
            return notification_description.compare( i->second ) == 0
                && severity == RS2_LOG_SEVERITY_ERROR;
        };
        REQUIRE( cv.wait_for( lock, std::chrono::seconds( 5 ), pred ) );
    }
}