// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

//#cmake:add-file log-common.h
#include "log-common.h"


TEST_CASE( "Mixed file & console logging", "[log]" ) {

    char filename[L_tmpnam];
    tmpnam( filename );

    TRACE( "Filename logging to: " << filename );
    REQUIRE_NOTHROW( rs2::log_to_file( RS2_LOG_SEVERITY_ERROR, filename ));
    REQUIRE_NOTHROW( rs2::log_to_console( RS2_LOG_SEVERITY_ERROR ));

    // Following should log to both, meaning the file should get one line:
    log_all();

    el::Loggers::flushAll();   // requires static!
    REQUIRE( count_lines( filename ) == 1 );
}
