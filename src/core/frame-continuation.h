// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.
#pragma once

#include <functional>


namespace librealsense {


class frame_continuation
{
    std::function< void() > continuation;
    const void * protected_data = nullptr;

    frame_continuation( const frame_continuation & ) = delete;
    frame_continuation & operator=( const frame_continuation & ) = delete;

public:
    frame_continuation()
        : continuation( []() {} )
    {
    }

    explicit frame_continuation( std::function< void() > continuation, const void * protected_data )
        : continuation( continuation )
        , protected_data( protected_data )
    {
    }


    frame_continuation( frame_continuation && other )
        : continuation( std::move( other.continuation ) )
        , protected_data( other.protected_data )
    {
        other.continuation = []() {
        };
        other.protected_data = nullptr;
    }

    void operator()()
    {
        continuation();
        continuation = []() {
        };
        protected_data = nullptr;
    }

    void reset()
    {
        protected_data = nullptr;
        continuation = []() {
        };
    }

    const void * get_data() const { return protected_data; }

    frame_continuation & operator=( frame_continuation && other )
    {
        continuation();
        protected_data = other.protected_data;
        continuation = other.continuation;
        other.continuation = []() {
        };
        other.protected_data = nullptr;
        return *this;
    }

    ~frame_continuation() { continuation(); }
};


}  // namespace librealsense
