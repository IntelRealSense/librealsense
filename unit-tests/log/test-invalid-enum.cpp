// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE( "Logging with invalid enum", "[log]" ) {

    size_t n_callbacks = 0;
    auto callback = [&]( rs2_log_severity severity, rs2::log_message const& msg )
    {
        ++n_callbacks;
        TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
    };

    rs2::log_to_callback( RS2_LOG_SEVERITY_ALL, callback );
    REQUIRE( !n_callbacks );
    REQUIRE_THROWS( rs2::log( rs2_log_severity( 10 ), "10" ) );  // Throws recoverable_exception, which will issue a log by itself!
    REQUIRE_THROWS( rs2::log( rs2_log_severity( -1 ), "-1" ) );  //     'invalid enum value for argument "severity"'
    REQUIRE( n_callbacks == 2 );
}
