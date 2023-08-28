// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <functional>
#include <mutex>
#include <memory>


namespace rsutils {


// Delayed initialization of T: the initializer function is called on first need of T.
//
template< class T >
class lazy
{
    mutable std::mutex _mtx;
    std::function< T() > _init;
    mutable std::unique_ptr< T > _ptr;

public:
    lazy()
        : _init(
            []()
            {
                T t{};
                return t;
            } )
    {
    }
    lazy( std::function< T() > initializer )
        : _init( std::move( initializer ) )
    {
    }

    bool is_initialized() const { return _ptr.operator bool(); }

    T * operator->() { return operate(); }
    const T * operator->() const { return operate(); }
    T & operator*() { return *operate(); }
    const T & operator*() const { return *operate(); }

    lazy( lazy && other ) noexcept
    {
        std::lock_guard< std::mutex > lock( other._mtx );
        _init = std::move( other._init );
        if( other.is_initialized() )
            _ptr = std::move( other._ptr );
    }

    lazy & operator=( std::function< T() > func ) noexcept { return *this = lazy< T >( std::move( func ) ); }

    lazy & operator=( lazy && other ) noexcept
    {
        std::lock_guard< std::mutex > lock1( _mtx );
        std::lock_guard< std::mutex > lock2( other._mtx );
        _init = std::move( other._init );
        if( other.is_initialized() )
            _ptr = std::move( other._ptr );
        return *this;
    }

    void reset() const
    {
        std::lock_guard< std::mutex > lock( _mtx );
        _ptr.reset();
    }

private:
    T * operate() const
    {
        std::lock_guard< std::mutex > lock( _mtx );
        if( ! is_initialized() )
            _ptr = std::unique_ptr< T >( new T( _init() ) );
        return _ptr.get();
    }
};


}  // namespace rsutils
