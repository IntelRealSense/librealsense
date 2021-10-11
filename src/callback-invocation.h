// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#include "small-heap.h"

#include <chrono>


namespace librealsense {


struct callback_invocation
{
    std::chrono::high_resolution_clock::time_point started;
    std::chrono::high_resolution_clock::time_point ended;
};

typedef small_heap< callback_invocation, 1 > callbacks_heap;

struct callback_invocation_holder
{
    callback_invocation_holder()
        : invocation( nullptr )
        , owner( nullptr )
    {
    }
    callback_invocation_holder( const callback_invocation_holder & ) = delete;
    callback_invocation_holder & operator=( const callback_invocation_holder & ) = delete;

    callback_invocation_holder( callback_invocation_holder && other )
        : invocation( other.invocation )
        , owner( other.owner )
    {
        other.invocation = nullptr;
    }

    callback_invocation_holder( callback_invocation * invocation, callbacks_heap * owner )
        : invocation( invocation )
        , owner( owner )
    {
    }

    ~callback_invocation_holder()
    {
        if( invocation )
            owner->deallocate( invocation );
    }

    callback_invocation_holder & operator=( callback_invocation_holder && other )
    {
        invocation = other.invocation;
        owner = other.owner;
        other.invocation = nullptr;
        return *this;
    }

    operator bool() { return invocation != nullptr; }

private:
    callback_invocation * invocation;
    callbacks_heap * owner;
};


}  // namespace librealsense
