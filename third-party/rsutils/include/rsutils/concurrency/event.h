// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2025 Intel Corporation. All Rights Reserved.
#pragma once

#include <chrono>
#include <mutex>
#include <condition_variable>


namespace rsutils {
namespace concurrency {


// A Python Event-like implementation, simplifying condition-variable and mutex access
//
class event
{
    mutable std::condition_variable _cv;
    mutable std::mutex _m;
    bool _is_set;

public:
    event( bool is_set = false )
        : _is_set( is_set )
    {
    }

    // Return true if the event was set()
    bool is_set() const
    {
        std::unique_lock< std::mutex > lock( _m );
        return _is_set;
    }

    operator bool() const { return is_set(); }

    // Trigger the event
    // Threads waiting on it will wake up
    void set()
    {
        {
            std::unique_lock< std::mutex > lock( _m );
            _is_set = true;
        }
        _cv.notify_all();
    }

    // Untrigger the event
    // Does not affect any threads
    // Returns the previous state
    bool clear()
    {
        std::unique_lock< std::mutex > lock( _m );
        bool was_set = _is_set;
        _is_set = false;
        return was_set;
    }

    // Block until the event is set()
    // If already set, returns immediately
    // The event remains set when returning: it needs to be cleared...
    void wait() const
    {
        std::unique_lock< std::mutex > lock( _m );
        if( ! _is_set )
            _cv.wait( lock );
    }

    // Clear and block until the event is set() again
    void clear_and_wait()
    {
        std::unique_lock< std::mutex > lock( _m );
        _is_set = false;
        _cv.wait( lock );
    }

    // Block until the event is set(), or the timeout occurs
    // If already set, returns immediately
    // Returns true unless a timeout occurred
    template< class Rep, class Period >
    bool wait( std::chrono::duration< Rep, Period > const & timeout ) const
    {
        std::unique_lock< std::mutex > lock( _m );
        return _is_set || std::cv_status::timeout != _cv.wait_for( lock, timeout );
    }

    // Clear and block until the event is set() again or timeout occurs
    // Returns true unless a timeout occurred
    template< class Rep, class Period >
    bool clear_and_wait( std::chrono::duration< Rep, Period > const & timeout )
    {
        std::unique_lock< std::mutex > lock( _m );
        _is_set = false;
        return std::cv_status::timeout != _cv.wait_for( lock, timeout );
    }
};


}  // namespace concurrency
}  // namespace rsutils
