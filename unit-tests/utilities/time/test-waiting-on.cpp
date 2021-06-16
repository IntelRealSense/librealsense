// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include <easylogging++.h>
#ifdef BUILD_SHARED_LIBS
// With static linkage, ELPP is initialized by librealsense, so doing it here will
// create errors. When we're using the shared .so/.dll, the two are separate and we have
// to initialize ours if we want to use the APIs!
INITIALIZE_EASYLOGGINGPP
#endif

#include <unit-tests/catch.h>
#include <common/utilities/time/waiting-on.h>
#include <queue>

using utilities::time::waiting_on;

bool invoke( size_t delay_in_thread, size_t timeout )
{
    waiting_on< bool > invoked( false );

    auto lambda = [delay_in_thread, invoked = invoked.in_thread()]() {
        //std::cout << "In thread" << std::endl;
        std::this_thread::sleep_for( std::chrono::seconds( delay_in_thread ));
        //std::cout << "Signalling" << std::endl;
        invoked.signal( true );
    };
    //std::cout << "Starting thread" << std::endl;
    std::thread( lambda ).detach();
    //std::cout << "Waiting" << std::endl;
    invoked.wait_until( std::chrono::seconds( timeout ), [&]() {
        return invoked;
        } );
    //std::cout << "After wait" << std::endl;
    return invoked;
}

TEST_CASE( "Basic wait" )
{
    REQUIRE( invoke( 1 /* seconds in thread */, 10 /* timeout */ ) );
}

TEST_CASE( "Timeout" )
{
    REQUIRE( ! invoke( 10 /* seconds in thread */, 1 /* timeout */ ));
}

TEST_CASE( "Struct usage" )
{
    struct value_t
    {
        double d;
        std::atomic_int i{ 0 };
    };
    waiting_on< value_t > output;
    output->d = 2.;

    std::thread( [output_ = output.in_thread()]() {
        auto p_output = output_.still_alive();
        auto& output = *p_output;
        while( output->i < 30 )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            ++output->i;
            output.signal();
        }
    } ).detach();

    // Within a second, i should reach ~20, but we'll ask it top stop when it reaches 10
    output.wait_until( std::chrono::seconds( 1 ), [&]() {
        return output->i == 10;
        } );

    auto i1 = output->i.load();
    CHECK( i1 >= 10 ); // the thread is still running!
    CHECK( i1 < 16 );
    //std::cout << "i1= " << i1 << std::endl;

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    auto i2 = output->i.load();
    CHECK( i2 > i1 ); // the thread is still running!
    //std::cout << "i2= " << i2 << std::endl;

    // Wait until it's done, ~30x50ms = 1.5 seconds total
    REQUIRE( output->i < 30 );
    output.wait_until( std::chrono::seconds( 3 ), [&]() { return false; } );
    REQUIRE( output->i == 30 );
}

TEST_CASE( "Not invoked but still notified" )
{
    // Emulate some dispatcher
    typedef std::function< void () > func;
    auto dispatcher = new std::queue< func >;

    // Push some stuff onto it (not important what)
    int i = 0;
    dispatcher->push( [&]() { ++i; } );
    dispatcher->push( [&]() { ++i; } );
    
    // Add something we'll be waiting on
    utilities::time::waiting_on< bool > invoked( false );
    dispatcher->push( [invoked_in_thread = invoked.in_thread()]() {
        invoked_in_thread.signal( true );
    } );

    // Destroy the dispatcher while we're waiting on the invocation!
    std::atomic_bool stopped( false );
    std::thread( [&]() {
        std::this_thread::sleep_for( std::chrono::seconds( 2 ));
        stopped = true;
        delete dispatcher;
        } ).detach();

    // Wait for it -- we'd expect that, when 'invoked_in_thread' is destroyed, it'll wake us up and
    // not wait for the timeout
    auto wait_start = std::chrono::high_resolution_clock::now();
    invoked.wait_until( std::chrono::seconds( 5 ), [&]() {
        return invoked || stopped;  // Without stopped, invoked will be false and we'll wait again
                                    // even after we're signalled!
        } );
    auto wait_end = std::chrono::high_resolution_clock::now();
    auto waited_ms = std::chrono::duration_cast<std::chrono::milliseconds>( wait_end - wait_start ).count();
#if 0
    // TODO: the requires below depend on the commented-out ~waiting_on::in_frame_(), but it also
    // causes unintended slowdowns when the playback is done
    REQUIRE( waited_ms > 4990 );
#else
    REQUIRE( waited_ms > 1990 );
    REQUIRE( waited_ms < 3000 );    // Up to a second buffer
#endif
}
