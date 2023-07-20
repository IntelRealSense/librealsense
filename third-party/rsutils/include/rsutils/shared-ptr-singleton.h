// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <mutex>    // std::mutex
#include <memory>   // std::shared_ptr
#include <utility>  // std::forward


namespace rsutils {


// Implements a singleton-like object, managing access to a shared_ptr, such that:
//     - defaults to nothing
//     - an instance is created, if it isn't already, on first call to instance()
//     - just like shared_ptr, when the last reference is removed, the underlying object is deleted
//     - keeping the singleton itself does not use a reference (unlike a shared_ptr)
//     - access is thread-safe
// Unlike an actual singleton:
//     - multiple instances can exist (usage is up to the user)
//
// For example, an actual singleton is simple to implement by declaring a global variable:
//     rsutils::shared_ptr_singleton< my_object > G_object;    // empty, not created
//     shared_ptr< my_object > obj_ptr = G_object.instance();  // creates the instance
//     obj_ptr.reset();                                        // destroys the instance
//
template< class T >
class shared_ptr_singleton
{
    // NOTE, see here:
    //     https://stackoverflow.com/questions/20705304/about-thread-safety-of-weak-ptr
    // and:
    //     https://en.cppreference.com/w/cpp/memory/weak_ptr/lock
    // lock() is executed atomically. We still "have to make sure not to modify the same weak_ptr from one thread while
    // accessing it from another"

    std::mutex _mutex;
    std::weak_ptr< T > _weak_ptr;

public:
    // Get it or an empty shared_ptr; don't create it
    // (note: lock() is executed atomically; no need to use the mutex)
    //
    std::shared_ptr< T > get() const { return _weak_ptr.lock(); }

    // Get it, or create it if it doesn't exist
    // Arguments are optional and are passed to the constructor if called
    //
    template< typename... Args >
    std::shared_ptr< T > instance( Args... args )
    {
        // The mutex is important when we have to actually create: without it we'll lock() atomically, then another
        // thread locks, and both get nulls so both try to create.
        std::lock_guard< std::mutex > lock( _mutex );
        std::shared_ptr< T > ptr = _weak_ptr.lock();
        if( ptr )
        {
            // The singleton is still alive and we can just use it
        }
        else
        {
            // First instance ever of T, or the singleton died (all references to the singleton were released), so
            // we have to recreate it
            ptr = std::make_shared< T >( std::forward< Args >( args )... );
            _weak_ptr = ptr;
        }
        return ptr;
    }

    // No is_valid(), operator!(), operator bool(), etc.: the result would be volatile and by the next line could be
    // invalid. Use get() instead and store the pointer until you're done with it!
};


}  // namespace rsutils
