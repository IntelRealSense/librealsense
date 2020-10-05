// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include <chrono>
namespace time_utilities
{
    class stopwatch
    {
    public:
        using clock = std::chrono::steady_clock;

        stopwatch()
        {
            _start = clock::now();
        }

        // Reset the stopwatch time
        void reset(clock::time_point start_time = clock::now()) const 
        { 
            _start = start_time; 
        }

        // Get elapsed in milliseconds since timer creation
        double get_elapsed_ms() const
        {
            return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(get_elapsed()).count();
        }

        // Get elapsed since timer creation
        clock::duration get_elapsed() const
        {
            return clock::now() - _start;
        }

        // Get stopwatch start time
        clock::time_point get_start() const
        {
            return _start;
        }

    protected:
        mutable clock::time_point _start;
    };
}
