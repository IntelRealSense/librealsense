// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>


namespace rsutils {
namespace time {

// Helper class -- encapsulate a variable of type T that we want to wait on: another thread will set
// it and signal when we can continue...
// 
// In order to synchronize the users predicate, we expect the user to provide his conditional variable and mutex used to set the predicate.
// As mentioned at the conditional variable documentation: (https://en.cppreference.com/w/cpp/thread/condition_variable)
//     The thread that intends to modify the shared variable has to
//      1. acquire a std::mutex(typically via std::lock_guard)
//      2. perform the modification while the lock is held
//      3. execute notify_one or notify_all on the std::condition_variable(the lock does not need to be held for notification)
// For more detailed information see https://www.modernescpp.com/index.php/c-core-guidelines-be-aware-of-the-traps-of-condition-variables
template< class T >
class waiting_on
{
    // We need to be careful with timeouts: if we time out then the local waiting_on can go out of
    // scope and then the thread cannot set anything or signal anyone! We get around this by using a
    // shared_ptr & weak_ptr to manage access:
    //
public:
    class wait_state_t
    {
        T _value;
        std::condition_variable &_cv;
        std::mutex &_m;
        std::atomic_bool _valid{ true };
        friend class waiting_on;

    public:
        wait_state_t() = delete; // Do not allow default Ctor, we need the user's CV and Mutex
        wait_state_t( std::condition_variable & cv, std::mutex & m )
            : _cv( cv )
            , _m( m )
        {
        }

        wait_state_t( std::condition_variable &cv, std::mutex &m, T const & t )
            : _cv( cv )
            , _m( m )
            , _value( t )
        {
        }

        operator T &() { return _value; }
        operator T const &() const { return _value; }

        T* operator->() { return &_value; }
        T const* operator->() const { return &_value; }

        // Set a new value and signal
        void signal( T const & t )
        {
            std::unique_lock<std::mutex> locker(_m);
            _value = t;
            locker.unlock();
            signal();
        }
        // Signal with the current value
        void signal()
        {
            _cv.notify_one();
        }
        // Invalidate the wait_state_t so the user will not use destroyed objects
        void invalidate()
        {
            if ( _valid )
            {
                _valid = false;
                _cv.notify_all();
            }
        }
    };
private:
    std::shared_ptr< wait_state_t > _ptr;

    // When we declare the signalling lambda for the other thread, we need to pass it the weak_ptr.
    // This class wraps it up nicely, so you can write:
    //     waiting_on< bool > invoked( false )
    //     auto thread_function = [invoked = invoked.in_thread()]() {
    //         invoked.signal( true );
    //     }
    //
public:
    class in_thread_
    {
        std::weak_ptr< wait_state_t > const _ptr;
        // We use an invalidator for invalidating the class when reference count is equal to Zero.
        std::shared_ptr< std::nullptr_t > const _invalidator;

    public:
        in_thread_( waiting_on const & local )
            : _ptr( local._ptr )
            , _invalidator(
                  nullptr,
                  [weak_ptr = std::weak_ptr< wait_state_t >( local._ptr )]( std::nullptr_t * ) {
                      // We get here when the lambda we're in is destroyed -- so either we've
                      // already run (and signalled once) or we've never run. We signal anyway
                      // if anything's waiting they'll get woken up; otherwise nothing'll happen...
                      if( auto wait_state = weak_ptr.lock() )
                          wait_state->invalidate();
                  } )
        {
        }

        std::shared_ptr< wait_state_t > still_alive() const { return _ptr.lock(); }

        // Wake up the local function (which is using wait_until(), presumable) with a new
        // T value
        void signal( T const& t ) const
        {
            if( auto wait_state = still_alive() )
                wait_state->signal( t );
        }
    };

public:
    waiting_on( std::condition_variable & cv, std::mutex & m )
        : _ptr( std::make_shared< wait_state_t >( cv, m ) )
    {
    }
    waiting_on( std::condition_variable & cv, std::mutex & m, T const & value )
        : _ptr( std::make_shared< wait_state_t >( cv, m, value ) )
    {
    }

    // Convert to the in-thread representation
    in_thread_ in_thread() const { return in_thread_( *this ); }

    operator T const &() const { return *_ptr; }
    
    // struct value_t { double x; int k; };
    // waiting_on< value_t > output({ 1., -1 });
    // output->x = 2.;
    T * operator->() { return &_ptr->_value; }
    T const * operator->() const { return &_ptr->_value; }

    // Wait until either the timeout occurs, or the predicate evaluates to true.
    // Equivalent to:
    //     while( ! pred() )
    //     {
    //         wait( timeout );
    //         if( timed-out )
    //             break;
    //     }
    template < class U, class L >
    void wait_until( U const& timeout, L const& pred )
    {
        // The CV must use the same lock that locks the predicate assignment value, otherwise it
        // could miss the signal which is a known trap of conditional variables
        std::unique_lock< std::mutex > locker(_ptr->_m);
        _ptr->_cv.wait_for( locker, timeout, [&]() -> bool {
            if( ! _ptr->_valid )
                return true;
            return pred();
        } );
    }
};


}  // namespace time
}  // namespace rsutils
