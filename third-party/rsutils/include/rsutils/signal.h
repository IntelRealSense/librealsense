// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <mutex>
#include <functional>


namespace rsutils {


template< typename... Args >
class signal
{
    std::mutex m_mutex;
    std::map< int, std::function< void( Args... ) > > m_subscribers;

    signal( const signal & other );        // non construction-copyable
    signal & operator=( const signal & );  // non copyable

public:
    signal() = default;

    signal( signal && other )
    {
        std::lock_guard< std::mutex > locker( other.m_mutex );
        m_subscribers = std::move( other.m_subscribers );

        other.m_subscribers.clear();
    }

    signal & operator=( signal && other )
    {
        std::lock_guard< std::mutex > locker( other.m_mutex );
        m_subscribers = std::move( other.m_subscribers );

        other.m_subscribers.clear();
        return *this;
    }

    int subscribe( const std::function< void( Args... ) > & func )
    {
        std::lock_guard< std::mutex > locker( m_mutex );

        int token = -1;
        for( int i = 0; i < ( std::numeric_limits< int >::max )(); i++ )
        {
            if( m_subscribers.find( i ) == m_subscribers.end() )
            {
                token = i;
                break;
            }
        }

        if( token != -1 )
        {
            m_subscribers.emplace( token, func );
        }

        return token;
    }

    bool unsubscribe( int token )
    {
        std::lock_guard< std::mutex > locker( m_mutex );

        bool retVal = false;
        typename std::map< int, std::function< void( Args... ) > >::iterator it = m_subscribers.find( token );
        if( it != m_subscribers.end() )
        {
            m_subscribers.erase( token );
            retVal = true;
        }

        return retVal;
    }

    bool raise( Args... args )
    {
        std::vector< std::function< void( Args... ) > > functions;
        bool retVal = false;

        std::unique_lock< std::mutex > locker( m_mutex );
        if( m_subscribers.size() > 0 )
        {
            typename std::map< int, std::function< void( Args... ) > >::iterator it = m_subscribers.begin();
            while( it != m_subscribers.end() )
            {
                functions.emplace_back( it->second );
                ++it;
            }
        }
        locker.unlock();

        if( functions.size() > 0 )
        {
            for( auto func : functions )
            {
                func( std::forward< Args >( args )... );
            }

            retVal = true;
        }

        return retVal;
    }
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
