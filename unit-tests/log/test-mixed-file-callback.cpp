// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE( "Mixed file & callback logging", "[log]" ) {

    char filename[L_tmpnam];
    tmpnam( filename );

    TRACE( "Filename logging to: " << filename );
    REQUIRE_NOTHROW( rs2::log_to_file( RS2_LOG_SEVERITY_ERROR, filename ));
    size_t n_callbacks = 0;
    REQUIRE_NOTHROW( rs2::log_to_callback( RS2_LOG_SEVERITY_ALL,
        [&]( rs2_log_severity severity, rs2::log_message const& msg )
        {
            ++n_callbacks;
            TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
        } ));

    // Following should log to both
    log_all();

    el::Loggers::flushAll();   // requires static!
    REQUIRE( count_lines( filename ) == 1 );

    REQUIRE( n_callbacks == 4 );
}
