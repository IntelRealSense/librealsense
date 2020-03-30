// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"


// See log_callback_function_ptr
size_t c_n_callbacks = 0;
void c_callback( rs2_log_severity severity, rs2_log_message const* msg, void * arg )
{
    REQUIRE( arg == (void*) 0xbadf00d );

    ++c_n_callbacks;
    rs2_error* e = nullptr;
    char const* str = rs2_get_full_log_message( msg, &e );
    REQUIRE_NOTHROW( rs2::error::handle( e ) );
    TRACE( str );
}


TEST_CASE( "Logging C ERROR", "[log]" ) {
    c_n_callbacks = 0;

    rs2_error* e = nullptr;
    rs2_log_to_callback( RS2_LOG_SEVERITY_ERROR, c_callback, ( void * ) 0xbadf00d, &e );
#if BUILD_EASYLOGGINGPP
    REQUIRE_NOTHROW( rs2::error::handle( e ) );
    REQUIRE( !c_n_callbacks );
    log_all();
    REQUIRE( c_n_callbacks == 1 );
#else //BUILD_EASYLOGGINGPP
    REQUIRE_THROWS(rs2::error::handle(e));
#endif //BUILD_EASYLOGGINGPP
}
