// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Has to be static or else our setenv won't affect LRS!
//#cmake: static!

//#cmake: add-file log-common.h
#include "log-common.h"
#include <stdlib.h>


TEST_CASE( "With LRS_LOG_LEVEL", "[log]" ) {

    REQUIRE( ! getenv( "LRS_LOG_LEVEL" ));
    putenv( "LRS_LOG_LEVEL=WARN" );
    REQUIRE( getenv( "LRS_LOG_LEVEL" ));
    TRACE( "LRS_LOG_LEVEL=" << getenv( "LRS_LOG_LEVEL" ));

    size_t n_callbacks = 0;
    auto callback = [&]( rs2_log_severity severity, rs2::log_message const& msg )
    {
        ++n_callbacks;
        TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
    };

    rs2::log_to_callback( RS2_LOG_SEVERITY_ERROR, callback );
    REQUIRE( !n_callbacks );
    log_all();
    // Without LRS_LOG_LEVEL, the result would be 1 (error)
    //REQUIRE( n_callbacks == 1 );
    // But, since we have the log-level forced to warning:
    REQUIRE( n_callbacks == 2 );
}
