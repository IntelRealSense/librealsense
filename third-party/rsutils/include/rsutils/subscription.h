// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#pragma once

#include "deferred.h"

#include <functional>
#include <memory>


namespace rsutils {


// Subscriptions can be temporary and so need to be removable. I.e., the subscriber needs to keep something (the
// "subscription") around so that it may be removed later on. This can be captured via some value, or an object with
// automatic destruction:
//
//     auto subscription = signal.subscribe( [] {} );
// 
// Note that cancellation is automatic on destruction, so it can be used in RAII.
// 
// Commonly used with signal.
//
class subscription
{
    deferred _d;

public:
    subscription() = default;

    subscription( deferred::fn && fn )
        : _d( std::move( fn ) )
    {
    }

    subscription( subscription && ) = default;

    // Assignment implies cancellation
    subscription & operator=( subscription && other )
    {
        cancel();
        _d = std::move( other._d );
        return *this;
    }

    // Unsubscribing is "canceling" the subscription. Will not throw if we've already cancelled.
    void cancel()
    {
        if( _d )
            _d.detach()();
    }

    // Allow removing the subscription (so it will never be "cancelled")
    deferred::fn detach()
    {
        return _d.detach();
    }
};


}  // namespace rsutils
