// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include <chrono>
#include "stopwatch.h"

namespace time_utilities
{
    class timer
    {
        
    public:
        using clock = std::chrono::steady_clock;

        timer(clock::duration timeout) : _delta(timeout) , sw()
        {
        }

        // Start timer
        void start() const
        {
            sw.reset();
        }

        // Check if timer time expired
        bool has_expired() const
        {
            return sw.get_start() + _delta <= clock::now();
        }

        // Force time expiration
        void set_expired() 
        {
            sw.reset(clock::now() - (_delta + std::chrono::nanoseconds(100)));
        }
    protected:
        clock::duration _delta;
        stopwatch sw;
    };
}
