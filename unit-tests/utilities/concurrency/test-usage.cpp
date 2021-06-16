// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#include "../../test.h"
#include <common/utilities/time/timer.h>
#include <src/concurrency.h>

using namespace utilities::time;

TEST_CASE( "dequeue wait after stop" )
{
    single_consumer_queue< std::function< void( void ) > > scq;
    timer t( std::chrono::seconds( 1 ) );
    std::function< void( void ) > f;
    std::function< void( void ) > * f_ptr = &f;

    scq.enqueue( []() {} );
    REQUIRE( scq.size() == 1 );
    REQUIRE( scq.peek( &f_ptr ) );

    REQUIRE( scq.started() );
    REQUIRE_FALSE( scq.stopped() );

    scq.stop();

    REQUIRE_FALSE( scq.peek( &f_ptr ) );
    REQUIRE( scq.stopped() );
    REQUIRE_FALSE( scq.started() );

    REQUIRE( scq.empty() );

    t.start();
    scq.dequeue( &f, 10000 );
    REQUIRE_FALSE( t.has_expired() );  // Verify no timeout, dequeue return in less than 10 seconds
}


TEST_CASE( "dequeue don't wait when queue is not empty" )
{
    single_consumer_queue< std::function< void( void ) > > scq;
    timer t( std::chrono::seconds( 1 ) );
    std::function< void( void ) > f;

    scq.enqueue( []() {} );
    t.start();
    scq.dequeue( &f, 3000 );
    REQUIRE_FALSE( t.has_expired() );  // Verify no timeout, dequeue return in less than 1 seconds
    REQUIRE( scq.empty() );
}

TEST_CASE( "dequeue wait when queue is empty" )
{
    single_consumer_queue< std::function< void( void ) > > scq;
    timer t( std::chrono::seconds( 1 ) );

    std::function< void( void ) > f;

    t.start();
    scq.dequeue( &f, 3000 );
    REQUIRE( t.has_expired() );  // Verify timeout, dequeue return after >= 3 seconds
}

TEST_CASE( "try dequeue" )
{
    single_consumer_queue< std::function< void( void ) > > scq;
    std::function< void( void ) > f;

    REQUIRE_FALSE( scq.try_dequeue( &f ) );  // nothing on queue
    scq.enqueue( []() {} );
    REQUIRE( scq.try_dequeue( &f ) );        // 1 item on queue
    REQUIRE_FALSE( scq.try_dequeue( &f ) );  // 0 items on queue
}

TEST_CASE( "blocking enqueue" )
{
    single_consumer_queue< std::function< void( void ) > > scq;
    std::function< void( void ) > f;
    stopwatch sw;

    sw.reset();

    // dequeue an item after 5 seconds, we wait for the blocking call and verify we return after this dequeue call ~ 5 seconds
    std::thread dequeue_thread( [&]() {
        std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
        scq.dequeue( &f, 1000 );
    } );

    REQUIRE( sw.get_elapsed_ms() < 1000 );

    for( int i = 0; i < 10; ++i )
    {
        scq.blocking_enqueue( []() {} );
    }
    REQUIRE( sw.get_elapsed_ms() < 1000 );
    REQUIRE( scq.size() == 10 );            // verify queue is full (default capacity is 10)
    scq.blocking_enqueue( []() {} );        // add a blocking call (item 11)
    REQUIRE( sw.get_elapsed_ms() > 5000 );  // verify the blocking call return on dequeue time
    REQUIRE( sw.get_elapsed_ms() < 6000 );  // verify the blocking call return on dequeue time

    dequeue_thread.join();  // verify clean exit with no threads alive.
}







