// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

//#cmake: shared!

//#cmake: add-file log-common.h
#include "log-common.h"

// Our EL++ instance is different than the rs2 one. Any LOG() calls will therefore
// use a different mechanism/callback than LRS's -- i.e., callbacks passed to LRS
// will not catach LOG() calls, and LRS won't catch our own calls either!

class test_fixture
{
    // Capture any of the local (our) EL++ LOG() calls
    class our_dispatcher : public el::LogDispatchCallback
    {
    public:
        size_t* pn_callbacks = nullptr;  // only the default ctor is available to us...!
    protected:
        void handle( const el::LogDispatchData* data ) noexcept override
        {
            (*pn_callbacks)++;
            TRACE( "OUR: " << data->logMessage()->logger()->logBuilder()->build( data->logMessage(), true ) );
        }
    };

protected:
    size_t n_callbacks_lrs = 0;
    size_t n_callbacks_our = 0;

public:
    test_fixture()
    {
        // Install two callbacks: one local ...
        el::Helpers::installLogDispatchCallback< our_dispatcher >( "our_dispatcher" );
        auto dispatcher = el::Helpers::logDispatchCallback< our_dispatcher >( "our_dispatcher" );
        dispatcher->pn_callbacks = &n_callbacks_our;
        // Make sure the default logger dispatch (which will log to standard out/err) is disabled
        el::Helpers::uninstallLogDispatchCallback< el::base::DefaultLogDispatchCallback >( "DefaultLogDispatchCallback" );

        // ... and one inside LRS
        auto callback = [&]( rs2_log_severity severity, rs2::log_message const& msg )
        {
            ++n_callbacks_lrs;
            TRACE( "LRS: " << severity << ' ' << msg.filename() << '+' << msg.line_number() << ": " << msg.raw() );
        };
        rs2::log_to_callback( RS2_LOG_SEVERITY_ALL, callback );
    }

    void reset()
    {
        n_callbacks_lrs = n_callbacks_our = 0;
    }
};


TEST_CASE_METHOD( test_fixture, "rs2::log vs LOG()", "[log]" ) {

    reset();

    SECTION( "lrs should get its own callbacks" ) {
        log_all();
        REQUIRE( n_callbacks_lrs == 4 );
        REQUIRE( n_callbacks_our == 0 );
    }
    SECTION( "default log; no callbacks" ) {
        LOG(INFO) << "LOG() message to our logger";
        REQUIRE( n_callbacks_lrs == 0 );
        REQUIRE( n_callbacks_our == 1 );
    }
    SECTION( "our logger is separate from librealsense logger" ) {
        // ("librealsense" is the name of the LRS logger; look in log.cpp)
        CLOG( INFO, "librealsense" ) << "LOG() message to \"librealsense\" logger";
        REQUIRE( n_callbacks_lrs == 0 );
        REQUIRE( n_callbacks_our == 1 );
    }
}
