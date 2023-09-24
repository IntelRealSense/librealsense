// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <map>
#include <mutex>
#include <functional>


namespace rsutils {


template< typename HostingClass, typename... Args >
class signal
{
    friend HostingClass;

public:
    signal() {}

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

    int operator+=( const std::function< void( Args... ) > & func ) { return subscribe( func ); }

    bool operator-=( int token ) { return unsubscribe( token ); }

private:
    signal( const signal & other );        // non construction-copyable
    signal & operator=( const signal & );  // non copyable

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

    bool operator()( Args... args ) { return raise( std::forward< Args >( args )... ); }

    std::mutex m_mutex;
    std::map< int, std::function< void( Args... ) > > m_subscribers;
};


}  // namespace rsutils
