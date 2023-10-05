// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include <rsutils/string/chrono.h>  // must be before catch.h!
#include <unit-tests/catch.h>
#include <rsutils/time/waiting-on.h>
#include <rsutils/time/timer.h>
#include <queue>

using rsutils::time::waiting_on;
using namespace rsutils::time;

bool invoke( size_t delay_in_thread, size_t timeout )
{
    std::condition_variable cv;
    std::mutex m;
    waiting_on< bool > invoked( cv, m, false );

    auto invoked_in_thread = invoked.in_thread();
    auto lambda = [delay_in_thread, invoked_in_thread]() {
        // std::cout << "In thread" << std::endl;
        std::this_thread::sleep_for( std::chrono::seconds( delay_in_thread ) );
        // std::cout << "Signalling" << std::endl;
        invoked_in_thread.signal( true );
    };
    // std::cout << "Starting thread" << std::endl;
    std::thread( lambda ).detach();
    // std::cout << "Waiting" << std::endl;
    invoked.wait_until( std::chrono::seconds( timeout ), [&]() { return invoked; } );
    // std::cout << "After wait" << std::endl;
    return invoked;
}

TEST_CASE( "Basic wait" )
{
    REQUIRE( invoke( 1 /* seconds in thread */, 10 /* timeout */ ) );
}

TEST_CASE( "Timeout" )
{
    REQUIRE( ! invoke( 10 /* seconds in thread */, 1 /* timeout */ ) );
}

TEST_CASE( "Struct usage" )
{
    struct value_t
    {
        double d;
        std::atomic_int i{ 0 };
    };

    std::condition_variable cv;
    std::mutex m;
    waiting_on< value_t > output( cv, m );
    output->d = 2.;

    auto output_ = output.in_thread();
    std::thread( [&m, output_]() {
        auto p_output = output_.still_alive();
        auto & output = *p_output;
        while( output->i < 30 )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 50 ) );
            std::lock_guard< std::mutex > lock( m );
            ++output->i;
            output.signal();
        }
    } ).detach();

    // Within a second, i should reach ~20, but we'll ask it top stop when it reaches 10
    output.wait_until( std::chrono::seconds( 1 ), [&]() { return output->i == 10; } );

    auto i1 = output->i.load();
    CHECK( i1 >= 10 );  // the thread is still running!
    CHECK( i1 < 16 );
    // std::cout << "i1= " << i1 << std::endl;

    std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
    auto i2 = output->i.load();
    CHECK( i2 > i1 );  // the thread is still running!
    // std::cout << "i2= " << i2 << std::endl;

    // Wait until it's done, ~30x50ms = 1.5 seconds total
    REQUIRE( output->i < 30 );
    output.wait_until( std::chrono::seconds( 3 ), [&]() { return false; } );
    REQUIRE( output->i == 30 );
}

TEST_CASE( "Not invoked but still notified by destructor" )
{
    // Emulate some dispatcher
    typedef std::function< void() > func;
    auto dispatcher = new std::queue< func >;

    // Push some stuff onto it (not important what)
    int i = 0;
    dispatcher->push( [&]() { ++i; } );
    dispatcher->push( [&]() { ++i; } );

    // Add something we'll be waiting on
    std::condition_variable cv;
    std::mutex m;
    rsutils::time::waiting_on< bool > invoked( cv, m, false );
    dispatcher->push(
        [invoked_in_thread = invoked.in_thread()]() { invoked_in_thread.signal( true ); } );

    // Destroy the dispatcher while we're waiting on the invocation!
    std::thread( [&]() {
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        delete dispatcher;
    } ).detach();

    // Wait for it -- we'd expect that, when 'invoked_in_thread' is destroyed, it'll wake us up and
    // not wait for the timeout
    stopwatch sw;
    invoked.wait_until( std::chrono::seconds( 5 ), [&]() { return invoked; } );
    auto waited = sw.get_elapsed();

    REQUIRE( waited > std::chrono::milliseconds( 1990 ) );
    REQUIRE( waited < std::chrono::milliseconds( 3000 ) );  // Up to a second buffer
}

TEST_CASE( "Not invoked but still notified by predicate (stopped)" )
{
    // Add something we'll be waiting on
    std::condition_variable cv;
    std::mutex m;
    rsutils::time::waiting_on< bool > invoked( cv, m, false );

    // Destroy the dispatcher while we're waiting on the invocation!
    std::atomic_bool stopped( false );
    std::thread( [&]() {
        std::this_thread::sleep_for( std::chrono::seconds( 2 ) );
        std::lock_guard< std::mutex > lock( m );
        stopped = true;
        cv.notify_all();
    } ).detach();

    // When 'stopped' is turned on , it'll wake us up and not wait for the timeout
    stopwatch sw;
    auto wait_start = std::chrono::high_resolution_clock::now();
    invoked.wait_until( std::chrono::seconds( 5 ), [&]() {
        return invoked || stopped;  // Without stopped, invoked will be false and we'll wait again
                                    // even after we're signalled!
    } );
    auto wait_end = std::chrono::high_resolution_clock::now();
    auto waited = sw.get_elapsed();

    REQUIRE( waited > std::chrono::milliseconds( 1990 ) );
    REQUIRE( waited < std::chrono::milliseconds( 3000 ) );  // Up to a second buffer
}

TEST_CASE( "Not invoked flush timeout expected" )
{
    // Add something we'll be waiting on
    std::condition_variable cv;
    std::mutex m;
    rsutils::time::waiting_on< bool > invoked( cv, m, false );

    stopwatch sw;
    auto timeout = std::chrono::seconds( 2 );
    invoked.wait_until( timeout, [&]() { return invoked; } );
    auto wait_time = sw.get_elapsed();

    INFO( wait_time.count() );
    INFO( timeout.count() );
    REQUIRE((wait_time >= timeout || to_string(wait_time) == to_string(timeout))); // timeout can occur slightly before what’s specified (1.9999975s) but as long as it translates to ‘2s’ we’re fine
}
