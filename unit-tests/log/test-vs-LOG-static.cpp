// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: static!

#include <rsutils/easylogging/easyloggingpp.h>
// Catch also defines CHECK(), and so we have to undefine it or we get compilation errors!
#undef CHECK
#include "../catch.h"
 
// With Catch2, turn this into SCOPED_INFO (right now, does not work)
#if 1
#define TRACE(X) do { \
    std::cout << X; \
    } while(0)
#else
#define TRACE(X) do {} while(0)
#endif

#include <src/log.h>


using namespace librealsense;


static void dummy_callback( rs2_log_severity, rs2_log_message const *, void * )
{
}


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
            if( pn_callbacks )
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
    LOG(INFO) << "Log message to default logger";
    CHECK( n_callbacks == 1 );

    // The librealsense logging mechanism is still off, so we don't get any callbacks

    CLOG( INFO, LIBREALSENSE_ELPP_ID ) << "Log message to \"librealsense\" logger";
    CHECK( n_callbacks == 1 );

    LOG_INFO( "Log message using LOG_INFO()" );
    CHECK( n_callbacks == 1 );

    REQUIRE_NOTHROW( rs2_log( RS2_LOG_SEVERITY_INFO, "Log message using rs2_log()", nullptr ) );
    CHECK( n_callbacks == 1 );

    // But once we turn it on...
    rs2_log_to_callback( RS2_LOG_SEVERITY_INFO, &dummy_callback, nullptr, nullptr );

    CLOG(INFO, LIBREALSENSE_ELPP_ID) << "Log message to \"librealsense\" logger";
    CHECK( n_callbacks == 2 );

    LOG_INFO( "Log message using LOG_INFO()" );
    CHECK( n_callbacks == 3 );

    REQUIRE_NOTHROW( rs2_log( RS2_LOG_SEVERITY_INFO, "Log message using rs2_log()", nullptr ));
    CHECK( n_callbacks == 4 );

    // NOTE that all the above called the same callback!! Callbacks are not logger-specific!
}
