// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE( "Double file logging", "[log]" ) {

    // Try to log to multiple destinations: callback, console, file...
    char filename1[L_tmpnam], filename2[L_tmpnam];
    tmpnam( filename1 );
    tmpnam( filename2 );

    TRACE( "Filename1 logging to: " << filename1 );
    TRACE( "Filename2 logging to: " << filename2 );
    REQUIRE_NOTHROW( rs2::log_to_file( RS2_LOG_SEVERITY_ERROR, filename1 ) );
    REQUIRE_NOTHROW( rs2::log_to_file( RS2_LOG_SEVERITY_ERROR, filename2 ) );

    // Following should log to only the latter!
    log_all();

    el::Loggers::flushAll();   // requires static!
    REQUIRE( count_lines( filename1 ) == 0 );
    REQUIRE( count_lines( filename2 ) == 1 );
}
