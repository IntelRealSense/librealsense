// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <unit-tests/catch.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/signal.h>
#include <ostream>


// Many of the ideas behind testing this came from:
// https://github.com/tarqd/slimsig/blob/master/test/test.cpp


TEST_CASE( "signal", "[signal]" )
{
    SECTION( "should trigger bound member function slots" )
    {
        struct C
        {
            bool bound_slot_triggered = false;
            void bound_slot() { bound_slot_triggered = true; }
        } obj;
        rsutils::signal<> signal;
        signal.add( std::bind( &C::bound_slot, &obj ) );
        signal.raise();
        CHECK( obj.bound_slot_triggered );
    }
    SECTION( "should trigger functor slots" )
    {
        static bool functor_slot_triggered = false;
        struct C
        {
            void operator()() { functor_slot_triggered = true; }
        } obj;
        rsutils::signal<> signal;
        signal.add( obj );  // makes a copy of obj!
        signal.raise();
        CHECK( functor_slot_triggered );
    }
    SECTION( "should trigger lambda slots" )
    {
        bool fired = false;
        rsutils::signal<> signal;
        signal.add( [&] { fired = true; } );
        signal.raise();
        CHECK( fired );
    }
}


TEST_CASE( "raise()", "[signal]" )
{
    SECTION( "should NOT perfectly forward r-value references" )
    {
        // Explanation: passing r-value refs (&&) will cause the first slot to be constructed
        // with SECTION -- essentially moving SECTION -- and the second slot to get nothing!
        rsutils::signal< std::string > signal;
        std::string str( "hello world" );
        signal.add( []( std::string str ) { CHECK( str == "hello world" ); } );
        signal.add( []( std::string str ) { CHECK( str == "hello world" ); } );
        signal.raise( std::move( str ) );
    }
    SECTION( "should not copy references" )
    {
        rsutils::signal< std::string & > signal;
        std::string str( "hello world" );
        signal.add(
            []( std::string & str )
            {
                CHECK( str == "hello world" );
                str = "hola mundo";
            } );
        signal.add( []( std::string & str ) { CHECK( str == "hola mundo" ); } );
        signal.raise( str );
    }
    SECTION( "should be re-entrant" )
    {
        unsigned count = 0;
        rsutils::signal<> signal;
        signal.add(
            [&]
            {
                ++count;
                if( count == 1 )
                {
                    signal.add( [&] { ++count; } );
                    signal.raise();
                };
            } );
        signal.raise();
        CHECK( count == 3 );
    }
}


TEST_CASE( "size()", "[signal]" )
{
    SECTION( "should return the subscription count" )
    {
        rsutils::signal<> signal;
        signal.add( [] {} );
        CHECK( signal.size() == 1 );
    }
    SECTION( "should return the correct count when adding slots during iteration" )
    {
        rsutils::signal<> signal;
        signal.add(
            [&]
            {
                signal.add( [] {} );
                CHECK( signal.size() == 2 );
            } );
        signal.raise();
        CHECK( signal.size() == 2 );
    }
}


TEST_CASE( "subscription", "[signal]" )
{
    rsutils::signal<> signal;

    SECTION( "cancel() should disconnect the slot" )
    {
        bool fired = false;
        auto subscription = signal.subscribe( [&] { fired = true; } );
        subscription.cancel();
        CHECK( signal.size() == 0 );
        signal.raise();
        CHECK_FALSE( fired );
    }
    SECTION( "cancel() should not throw if already disconnected" )
    {
        auto subscription = signal.subscribe( [] {} );
        subscription.cancel();
        subscription.cancel();
        CHECK( signal.size() == 0 );
    }
    SECTION( "should automatically remove the slot" )
    {
        bool fired = false;
        auto fn = [&] { fired = true; };
        {
            auto subscription = signal.subscribe( fn );
        }
        CHECK( signal.size() == 0 );
        signal.raise();
        CHECK_FALSE( fired );
    }
    SECTION( "unless detached" )
    {
        bool fired = false;
        auto fn = [&] { fired = true; };
        {
            signal.subscribe( fn ).detach();
        }
        CHECK( signal.size() == 1 );
        signal.raise();
        CHECK( fired );
    }
    SECTION( "should still be valid if the signal is destroyed" )
    {
        rsutils::subscription subscription;
        {
            rsutils::signal<> scoped_signal;
            subscription = scoped_signal.subscribe( [] {} );
        }
    }
    SECTION( "replacing subscription with another should cancel the previous" )
    {
        std::string val;
        auto subscription = signal.subscribe( [&val] { val += '1'; } );
        CHECK( signal.size() == 1 );
        signal.raise();
        CHECK( val == "1" );
        subscription = signal.subscribe( [&val] { val += '2'; } );
        CHECK( signal.size() == 1 );
        signal.raise();
        CHECK( val == "12" );
        subscription = {};
        CHECK( signal.size() == 0 );
    }
    SECTION( "detach() followed by cancel()" )
    {
        std::string val;
        auto subscription = signal.subscribe( [&val] { val += '1'; } );
        CHECK( signal.size() == 1 );
        subscription.detach();  // should no longer be valid
        CHECK( signal.size() == 1 );
        subscription.cancel();        // make sure!
        CHECK( signal.size() == 1 );  // nothing should have happened!
    }
}


TEST_CASE( "stress test", "[signal]" )
{
    rsutils::signal< int & > signal;
    constexpr size_t N = 100000;

    // add subscriptions
    {
        auto before = std::chrono::high_resolution_clock::now();
        for( int expected = 0; expected < N; ++expected )
        {
            signal.add(
                [expected]( int & i )
                {
                    CHECK( i == expected );
                    ++i;
                } );
        }
        auto after = std::chrono::high_resolution_clock::now();
        std::cout << "      took " << ((after - before).count() / 1e9) << " seconds to add" << std::endl;
        CHECK( signal.size() == N );
        int i = 0;
        before = std::chrono::high_resolution_clock::now();
        signal.raise( i );
        after = std::chrono::high_resolution_clock::now();
        std::cout << "      took " << ((after - before).count() / 1e9) << " seconds to raise" << std::endl;
    }
    // remove all but last
    {
        for( int pos = 0; pos < N - 1; ++pos )
        {
            CAPTURE( pos );
            CHECK( signal.remove( pos ) == true );
        }
        CHECK( signal.size() == 1 );
        int i = int( N ) - 1;  // the only one left
        signal.raise( i );
    }
    // add one -> should iterate AFTER the last
    {
        auto slot_id = signal.add( []( int & i ) { i = -25; } );
        int i = int( N ) - 1;  // the only one left
        signal.raise( i );
        CHECK( i == -25 );
    }
}
