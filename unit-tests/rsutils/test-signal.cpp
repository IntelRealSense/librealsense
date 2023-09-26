// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <unit-tests/catch.h>
#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/signal.h>
#include <ostream>


// Many of the ideas behind testing this came from:
// https://github.com/tarqd/slimsig/blob/master/test/test.cpp


// using subscription = typename rsutils::subscription;


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
        signal.subscribe( std::bind( &C::bound_slot, &obj ) );
        signal.raise();
        CHECK( obj.bound_slot_triggered );
    }
    SECTION( "should trigger functor slots" )
    {
        struct C
        {
            bool functor_slot_triggered = false;
            void operator()() { functor_slot_triggered = true; }
        } obj;
        rsutils::signal<> signal;
        signal.subscribe( obj );
        signal.raise();
        CHECK( obj.functor_slot_triggered );
    }
    SECTION( "should trigger lambda slots" )
    {
        bool fired = false;
        rsutils::signal<> signal;
        signal.subscribe( [&] { fired = true; } );
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
        signal.subscribe( []( std::string str ) { CHECK( str == "hello world" ); } );
        signal.subscribe( []( std::string str ) { CHECK( str == "hello world" ); } );
        signal.raise( std::move( str ) );
    }
    SECTION( "should not copy references" )
    {
        rsutils::signal< std::string & > signal;
        std::string str( "hello world" );
        signal.subscribe(
            []( std::string & str )
            {
                CHECK( str == "hello world" );
                str = "hola mundo";
            } );
        signal.subscribe( []( std::string & str ) { CHECK( str == "hola mundo" ); } );
        signal.raise( str );
    }
    SECTION( "should be re-entrant" )
    {
        unsigned count = 0;
        rsutils::signal<> signal;
        signal.subscribe(
            [&]
            {
                ++count;
                if( count == 1 )
                {
                    signal.subscribe( [&] { ++count; } );
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
        signal.subscribe( [] {} );
        CHECK( signal.size() == 1 );
    }
    SECTION( "should return the correct count when adding slots during iteration" )
    {
        rsutils::signal<> signal;
        signal.subscribe(
            [&]
            {
                signal.subscribe( [] {} );
                CHECK( signal.size() == 2 );
            } );
        signal.raise();
        CHECK( signal.size() == 2 );
    }
}


    /*
    describe("tracking", [] {
      rsutils::signal<void()> signal;
      before_each([&] { signal = rsutils::signal<void()>{}; });
      SECTION("should disconnect slots when tracked objects are destroyed", [&] {
        struct foo{};
        bool called = false;
        auto tracked = std::make_shared<foo>();
        signal.subscribe([&] {
          called = true;
        }, {tracked});
        tracked.reset();
        signal.raise();
        CHECK(called, ==(false));
        signal.compact();
        CHECK(signal.size(), ==(0));
      });
    });*/
#if 0
    describe( "connection",
              []
              {
                  rsutils::signal< void() > signal;
                  before_each( [&] { signal = rsutils::signal< void() >{}; } );
                  describe( "#connected()",
                            [&]
                            {
                                SECTION( "should return whether or not the slot is connected",
                                    [&]
                                    {
                                        auto connection = signal.subscribe( [] {} );
                                        CHECK( connection.connected(), == (true) );
                                        signal.disconnect_all();
                                        CHECK( connection.connected(), == (false) );
                                    } );
                            } );
                  describe( "#disconnect",
                            [&]
                            {
                                SECTION( "should disconnect the slot",
                                    [&]
                                    {
                                        bool fired = false;
                                        auto connection = signal.subscribe( [&] { fired = true; } );
                                        connection.disconnect();
                                        signal.raise();
                                        CHECK( fired, == (false) );
                                        CHECK( connection.connected(), == (false) );
                                        CHECK( signal.size(), == (0u) );
                                    } );
                                SECTION( "should not throw if already disconnected",
                                    [&]
                                    {
                                        auto connection = signal.subscribe( [] {} );
                                        connection.disconnect();
                                        connection.disconnect();
                                        CHECK( connection.connected(), == (false) );
                                        CHECK( signal.size(), == (0u) );
                                    } );
                            } );
                  SECTION( "should be consistent across copies",
                      [&]
                      {
                          auto conn1 = signal.subscribe( [] {} );
                          auto conn2 = conn1;
                          conn1.disconnect();
                          CHECK( conn1.connected(), == (conn2.connected()) );
                          CHECK( signal.size(), == (0u) );
                      } );
                  SECTION( "should not affect slot lifetime",
                      [&]
                      {
                          bool fired = false;
                          auto fn = [&]
                          {
                              fired = true;
                          };
                          {
                              auto connection = signal.subscribe( fn );
                          }
                          signal.raise();
                          CHECK( fired, == (true) );
                      } );
                  SECTION( "should still be valid if the signal is destroyed",
                      [&]
                      {
                          using connection_type = rsutils::signal< void() >::connection;
                          connection_type connection;
                          {
                              rsutils::signal< void() > scoped_signal{};
                              connection = scoped_signal.subscribe( [] {} );
                          }
                          CHECK( connection.connected(), == (false) );
                      } );
              } );
    describe( "scoped_connection",
              []
              {
                  rsutils::signal< void() > signal;
                  before_each( [&] { signal = rsutils::signal< void() >(); } );
                  SECTION( "should disconnect the connection after leaving the scope",
                      [&]
                      {
                          bool fired = false;
                          {
                              auto scoped = make_scoped_connection( signal.subscribe( [&] { fired = true; } ) );
                          }
                          signal.raise();
                          CHECK( fired, == (false) );
                          CHECK( signal.empty(), == (true) );
                      } );
                  SECTION( "should update state of underlying connection",
                      [&]
                      {
                          auto connection = signal.subscribe( [] {} );
                          {
                              auto scoped = make_scoped_connection( connection );
                          }
                          signal.raise();
                          CHECK( connection.connected(), == (false) );
                      } );
              } );
#endif


TEST_CASE( "stress test", "[signal]" )
{
    rsutils::signal< int & > signal;
    constexpr size_t N = 100000;

    // add subscriptions
    {
        for( int expected = 0; expected < N; ++expected )
        {
            signal.subscribe(
                [expected]( int & i )
                {
                    CHECK( i == expected );
                    ++i;
                } );
        }
        CHECK( signal.size() == N );
        int i = 0;
        auto before = std::chrono::high_resolution_clock::now();
        signal.raise( i );
        auto after = std::chrono::high_resolution_clock::now();
        auto delta = after - before;
        std::cout << "      took " << delta.count() / 1000. << " milliseconds total" << std::endl;
    }
    // remove all but last
    {
        for( int pos = 0; pos < N - 1; ++pos )
        {
            CAPTURE( pos );
            CHECK( signal.unsubscribe( pos ) == true );
        }
        CHECK( signal.size() == 1 );
        int i = int( N ) - 1;  // the only one left
        signal.raise( i );
    }
    // add one -> should iterate AFTER the last
    {
        auto slot_id = signal.subscribe( []( int & i ) { i = -25; } );
        int i = int( N ) - 1;  // the only one left
        signal.raise( i );
        CHECK( i == -25 );
    }
}
