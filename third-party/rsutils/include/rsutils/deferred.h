// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include <functional>


namespace rsutils {


// RAII for a function: calls it on destruction, allowing automatic calls when going out of scope very similar to a
// smart pointer that automatically deletes when going out of scope.
// 
class deferred
{
public:
    typedef std::function< void() > fn;

private:
    fn _deferred;

    deferred( const deferred & ) = delete;
    deferred & operator=( const deferred & ) = delete;

public:
    deferred() = default;

    deferred( fn && f )
        : _deferred( f )
    {
    }

    deferred( deferred && that ) = default;

    deferred & operator=( deferred && ) = default;

    bool is_valid() const { return ! ! _deferred; }
    operator bool() const { return is_valid(); }

    // Destructor forces deferred call to be executed
    ~deferred()
    {
        if( is_valid() )
            execute();
    }

    // Prevent the deferred call from ever being invoked
    void reset() { _deferred = {}; }

    // Returns the deferred function so you can move it somewhere else
    fn detach() { return std::move( _deferred ); }

    // Cause the deferred call to be invoked NOW (throws if nothing there!)
    void execute() { _deferred(); }
};


}  // namespace rsutils