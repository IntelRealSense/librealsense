// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake:add-file log-common.h
#include "log-common.h"


// See log_callback_function_ptr
size_t c_n_callbacks = 0;
void c_callback( rs2_log_severity severity, rs2_log_message const* msg )
{
    ++c_n_callbacks;
    rs2_error* e = nullptr;
    rs2_string* str_ptr = rs2_build_log_message( msg, &e );
    REQUIRE_NOTHROW( rs2::error::handle( e ));
    std::string str;
    REQUIRE_NOTHROW( str = rs2::get_string( str_ptr ));
    REQUIRE_NOTHROW( rs2_free_string( str_ptr ));
    TRACE( str );
}


TEST_CASE( "Logging C INFO", "[log]" ) {
    c_n_callbacks = 0;

    rs2_error* e = nullptr;
    rs2_log_to_callback( RS2_LOG_SEVERITY_INFO, c_callback, &e );
    REQUIRE_NOTHROW( rs2::error::handle( e ) );
    REQUIRE( !c_n_callbacks );
    log_all();
    REQUIRE( c_n_callbacks == 3 );
}
