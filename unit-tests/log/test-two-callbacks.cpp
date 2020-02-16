// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE( "Logging to two callbacks", "[log]" ) {

    size_t n_callbacks_1 = 0;
    auto callback1 = [&]( rs2_log_severity severity, rs2::log_message const& msg )
    {
        ++n_callbacks_1;
        TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
    };
    size_t n_callbacks_2 = 0;
    auto callback2 = [&]( rs2_log_severity severity, rs2::log_message const& msg )
    {
        ++n_callbacks_2;
        TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
    };

    REQUIRE_NOTHROW( rs2::log_to_callback( RS2_LOG_SEVERITY_ERROR, callback1 ));
    REQUIRE_NOTHROW( rs2::log_to_callback( RS2_LOG_SEVERITY_ERROR, callback2 ));

    REQUIRE( !n_callbacks_1 );
    REQUIRE( !n_callbacks_2 );
    log_all();  // one error should go to each
    REQUIRE( n_callbacks_1 == 1 );
    REQUIRE( n_callbacks_2 == 1 );
}
