// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "librealsense-exception.h"

#include <mutex>
#include <condition_variable>


namespace librealsense {


template < class T, int C >
class small_heap
{
    T buffer[C];
    bool is_free[C];
    std::mutex mutex;
    bool keep_allocating = true;
    std::condition_variable cv;
    int size = 0;

public:
    static const int CAPACITY = C;

    small_heap()
    {
        for( auto i = 0; i < C; i++ )
        {
            is_free[i] = true;
            buffer[i] = std::move( T() );
        }
    }

    T * allocate()
    {
        std::unique_lock< std::mutex > lock( mutex );
        if( ! keep_allocating )
            return nullptr;

        for( auto i = 0; i < C; i++ )
        {
            if( is_free[i] )
            {
                is_free[i] = false;
                size++;
                return &buffer[i];
            }
        }
        return nullptr;
    }

    void deallocate( T * item )
    {
        if( item < buffer || item >= buffer + C )
        {
            throw invalid_value_exception( "Trying to return item to a heap that didn't allocate it!" );
        }
        auto i = item - buffer;
        auto old_value = std::move( buffer[i] );
        buffer[i] = std::move( T() );

        {
            std::unique_lock< std::mutex > lock( mutex );

            is_free[i] = true;
            size--;

            if( size == 0 )
            {
                lock.unlock();
                cv.notify_one();
            }
        }
    }

    void stop_allocation()
    {
        std::unique_lock< std::mutex > lock( mutex );
        keep_allocating = false;
    }

    void wait_until_empty()
    {
        std::unique_lock< std::mutex > lock( mutex );

        const auto ready = [this]() {
            return is_empty();
        };
        if( ! ready()
            && ! cv.wait_for( lock,
                              std::chrono::hours( 1000 ),
                              ready ) )  // for some reason passing std::chrono::duration::max makes it return instantly
        {
            throw invalid_value_exception( "Could not flush one of the user controlled objects!" );
        }
    }

    bool is_empty() const { return size == 0; }
    int get_size() const { return size; }
};


}  // namespace librealsense
