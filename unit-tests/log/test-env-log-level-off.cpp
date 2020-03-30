// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"
#include <stdlib.h>


TEST_CASE( "Without LRS_LOG_LEVEL", "[log]" ) {

    REQUIRE( ! getenv( "LRS_LOG_LEVEL" ));

    size_t n_callbacks = 0;
    auto callback = [&]( rs2_log_severity severity, rs2::log_message const& msg )
    {
        ++n_callbacks;
        TRACE( severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
    };

#if BUILD_EASYLOGGINGPP
    rs2::log_to_callback( RS2_LOG_SEVERITY_ERROR, callback );
    REQUIRE( !n_callbacks );
    log_all();
    REQUIRE( n_callbacks == 1 );
#else //BUILD_EASYLOGGINGPP
    REQUIRE_THROWS(rs2::log_to_callback(RS2_LOG_SEVERITY_ERROR, callback));
#endif //BUILD_EASYLOGGINGPP
}
