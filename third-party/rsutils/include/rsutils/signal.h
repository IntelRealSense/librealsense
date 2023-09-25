// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <mutex>
#include <functional>


namespace rsutils {


// Outside signal<> so can be easy to reference - they should be the same regardless of the type of signal
using signal_slot = int;


// Signals are callbacks with multiple targets.
// 
// Each "slot" is a subscriber that's called when the signal is "raised."
// 
// There are intentionally no operators here (especially operator()!) as, despite the nice-to-look-at style, are not
// verbose enough to be really useful.
// 
// The template Args are the callback arguments, and therefore what's expected to be passed to raise(). The callbacks
// return void on purpose, for simplicity.
//
template< typename... Args >
class signal
{
    using callback = std::function< void( Args... ) >;

    std::mutex _mutex;
    std::map< signal_slot, callback > _subscribers;

    signal( const signal & other ) = delete;
    signal & operator=( const signal & ) = delete;

public:
    signal() = default;

    signal( signal && other )
    {
        std::lock_guard< std::mutex > locker( other._mutex );
        _subscribers = std::move( other._subscribers );
    }

    signal & operator=( signal && other )
    {
        std::lock_guard< std::mutex > locker( other._mutex );
        _subscribers = std::move( other._subscribers );
        return *this;
    }

    signal_slot subscribe( const callback && func )
    {
        std::lock_guard< std::mutex > locker( _mutex );
        // NOTE: we should maintain ordering of subscribers: later subscriptions should be called after earlier ones, so
        // the key should keep increasing in value
        auto key = _subscribers.empty() ? 0 : ( _subscribers.rbegin()->first + 1 );
        _subscribers.emplace( key, std::move( func ) );
        return key;
    }

    bool unsubscribe( signal_slot token )
    {
        std::lock_guard< std::mutex > locker( _mutex );
        return _subscribers.erase( token );
    }

    void raise( Args... args )
    {
        std::vector< callback > functions;

        {
            std::lock_guard< std::mutex > locker( _mutex );
            functions.reserve( _subscribers.size() );
            for( auto const & s : _subscribers )
                functions.push_back( s.second );
        }

        for( auto const & func : functions )
            func( std::forward< Args >( args )... );
    }

    // How many subscriptions are active
    size_t size() const { return _subscribers.size(); }
};


// This is what was called 'signal' before: it is a way to expose the signal as an actual public interface from your
// class. Rather than:
//      class X
//      {
//          signal< ... > _signal;
// 
//      public:
//          int set_callback( std::function< ... > && callback ) { return _signal.subscribe( std::move( callback ); }
//      };
// You can write:
//      class X
//      {
//      public:
//          public_signal< X, ... > callbacks;
//      };
// And:
//      X x;
//      x.callbacks.subscribe( []() { ... } );
// 
// The client (the one using X) should be able to subscribe/ubsubscribe, but not to raise callbacks. This is done by
// adding access specifiers such that raising is private and only a friend 'HostingClass' can raise:
//
template< typename HostingClass, typename... Args >
class public_signal : public signal< Args... >
{
    typedef signal< Args... > super;

public:
    using super::subscribe;
    using super::unsubscribe;

private:
    friend HostingClass;

    using super::raise;
};


}  // namespace rsutils
