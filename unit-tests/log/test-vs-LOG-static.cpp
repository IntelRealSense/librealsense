// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include <easylogging++.h>
// Catch also defines CHECK(), and so we have to undefine it or we get compilation errors!
#undef CHECK
#define CATCH_CONFIG_MAIN
#include "../catch/catch.hpp"
 
// With Catch2, turn this into SCOPED_INFO (right now, does not work)
#if 1
#define TRACE(X) do { \
    std::cout << X; \
    } while(0)
#else
#define TRACE(X) do {} while(0)
#endif

#include "../src/log.h"


using namespace librealsense;


TEST_CASE( "rs2_log vs LOG() - internal", "[log]" )
{
    size_t n_callbacks = 0;
    class default_dispatcher : public el::LogDispatchCallback
    {
    public:
        size_t* pn_callbacks = nullptr;  // only the default ctor is available to us...!
    protected:
        void handle( const el::LogDispatchData* data ) noexcept override
        {
            (*pn_callbacks)++;
            TRACE( data->logMessage()->logger()->logBuilder()->build( data->logMessage(), true ));
        }
    };
    el::Helpers::installLogDispatchCallback< default_dispatcher >( "default_dispatcher" );
    auto dispatcher = el::Helpers::logDispatchCallback< default_dispatcher >( "default_dispatcher" );
    dispatcher->pn_callbacks = &n_callbacks;
    // Make sure the default logger dispatch (which will log to standard out/err) is disabled
    el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );

    // LOG(XXX) should log to the default logger, which is NOT the librealsense logger
    REQUIRE_NOTHROW( LOG(INFO) << "Log message to default logger" );
    REQUIRE( n_callbacks == 1 );

    // CLOG(XXX,"librealsense") is the librealsense logger
    REQUIRE_NOTHROW( CLOG(INFO, "librealsense") << "Log message to \"librealsense\" logger" );
    REQUIRE( n_callbacks == 2 );

    // LOG_XXX() is same as CLOG( ..., "librealsense" )
    REQUIRE_NOTHROW( LOG_INFO( "Log message using LOG_INFO()" ) );
    REQUIRE( n_callbacks == 3 );

    // LOG_XXX() is same as CLOG( ..., "librealsense" )
    REQUIRE_NOTHROW( rs2_log( RS2_LOG_SEVERITY_INFO, "Log message using rs2_log()", nullptr ));
    REQUIRE( n_callbacks == 4 );

    // NOTE that all the above called the same callback!! Callbacks are not logger-specific!
}
