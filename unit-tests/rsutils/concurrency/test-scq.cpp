// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

//#cmake:dependencies rsutils

#include <unit-tests/test.h>
#include <rsutils/time/timer.h>
#include <rsutils/concurrency/concurrency.h>

#include <algorithm>
#include <vector>

using namespace rsutils::time;

TEST_CASE( "dequeue doesn't wait after stop" )
{
    typedef std::function< void( void ) > scq_value_t;
    single_consumer_queue< scq_value_t > scq;
    scq_value_t f;
    scq_value_t * f_ptr = &f;

    scq.enqueue( []() {} );
    REQUIRE( scq.size() == 1 );
    REQUIRE( scq.peek( [&]( scq_value_t const & ) {} ));

    REQUIRE( scq.started() );
    REQUIRE_FALSE( scq.stopped() );

    scq.stop();

    REQUIRE_FALSE( scq.peek( [&]( scq_value_t const& ) {} ) );
    REQUIRE( scq.stopped() );
    REQUIRE_FALSE( scq.started() );

    REQUIRE( scq.empty() );

    timer t( std::chrono::seconds( 1 ) );
    t.start();
    scq.dequeue( &f, 2000 );
    REQUIRE_FALSE( t.has_expired() );  // Verify no timeout, dequeue return in less than 10 seconds
}


TEST_CASE( "dequeue doesn't wait when queue is not empty" )
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
    timer t( std::chrono::milliseconds( 2900 ) );

    std::function< void( void ) > f;

    t.start();
    REQUIRE_FALSE( scq.dequeue( &f, 3000 ) );
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

TEST_CASE("verify mutex protection")
{
    single_consumer_queue< int > scq;
    stopwatch sw;

    const int MAX_SIZE_FOR_THREAD = 20;
    std::thread enqueue_thread1( [&]() {
        for( int i = 0; i < MAX_SIZE_FOR_THREAD; ++i )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            scq.blocking_enqueue(std::move(i));
        }
    } );

    std::thread enqueue_thread2( [&]() {
        for( int i = 0; i < MAX_SIZE_FOR_THREAD; ++i )
        {
            std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );
            scq.blocking_enqueue(std::move(20 + i));
        }
    } );

    std::vector<int> all_values;
    for( int i = 0; i < MAX_SIZE_FOR_THREAD * 2; ++i )
    {
        int val;
        scq.dequeue( &val, 1000 );
        all_values.push_back(val);
    }

    REQUIRE(all_values.size() == MAX_SIZE_FOR_THREAD * 2);

    std::sort(all_values.begin(), all_values.end());

    for (int i = 0; i < all_values.size(); ++i)
    {
        REQUIRE(all_values[i] == i);
    }

    enqueue_thread1.join();
    enqueue_thread2.join();
}
