// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "subscription.h"

#include <map>
#include <mutex>
#include <functional>
#include <memory>
#include <vector>


namespace rsutils {


// Outside signal<> so can be easy to reference - they should be the same regardless of the type of signal
using subscription_slot = int;


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
// NOTE: there are more efficient implementations (e.g., with a vector). We kept it map-based for simplicity.
//
template< typename... Args >
class signal
{
public:
    using callback = std::function< void( Args... ) >;
private:
    using map = std::map< subscription_slot, callback >;

    // We use a shared_ptr to control lifetime: only while it's alive can we remove subscriptions
    struct impl
    {
        std::mutex mutex;
        map subscribers;

        bool remove( subscription_slot slot )
        {
            std::lock_guard< std::mutex > locker( mutex );
            return subscribers.erase( slot );
        }
    };
    std::shared_ptr< impl > const _impl;

    signal( const signal & other ) = delete;
    signal & operator=( const signal & ) = delete;

public:
    signal() : _impl( std::make_shared< impl >() ) {}
    signal( signal && ) = default;
    signal & operator=( signal && ) = default;

    // Get the slot without any fancy subscription
    subscription_slot add( callback && func )
    {
        std::lock_guard< std::mutex > locker( _impl->mutex );
        // NOTE: we should maintain ordering of subscribers: later subscriptions should be called after earlier ones, so
        // the key should keep increasing in value
        auto slot = _impl->subscribers.empty() ? 0 : (_impl->subscribers.rbegin()->first + 1);
        _impl->subscribers.emplace( slot, std::move( func ) );
        return slot;
    }

    // Opposite of add()
    bool remove( subscription_slot slot ) { return _impl->remove( slot ); }

    // Unlike add(), subscribing implies a subscription, meaning the caller needs to store the return value or else it
    // will get unsubscribed immediately!!
    subscription subscribe( callback && func )
    {
        auto slot = add( std::move( func ) );
        return subscription(
            [slot, weak = std::weak_ptr< impl >( _impl )]
            {
                if( auto impl = weak.lock() )
                    impl->remove( slot );
            } );
    }

    bool raise( Args... args )
    {
        std::unique_lock< std::mutex > locker( _impl->mutex );
        auto const N = _impl->subscribers.size();
        if( ! N )
            return false;  // no notifications generated

        std::vector< callback > functions;
        functions.reserve( N );
        for( auto const & s : _impl->subscribers )
            functions.push_back( s.second );

        locker.unlock();

        // NOTE: when calling our subscribers, we do not perfectly forward on purpose to avoid the situation where the
        // first subscriber will move an argument and the second will then get nothing!
        //
        for( auto const & func : functions )
            func( /*std::forward< Args >(*/ args /*)*/... );

        return true;  // notifications went out
    }

    // How many subscriptions are active
    size_t size() const { return _impl->subscribers.size(); }
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
    using super::add;
    using super::remove;

private:
    friend HostingClass;

    using super::raise;
};


}  // namespace rsutils
