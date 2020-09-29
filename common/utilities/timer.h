// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include <chrono>
namespace time_utilities
{
    using clock = std::chrono::steady_clock;

    class stopwatch
    {
    public:
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
        double elapsed_ms() const
        {
            return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed()).count();
        }

        // Get elapsed since timer creation
        clock::duration elapsed() const
        {
            return clock::now() - _start;
        }

        // Get current absolute time
        clock::time_point now() const
        {
            return clock::now();
        }

        // Get stopwatch start time
        clock::time_point get_start_time() const
        {
            return _start;
        }

    protected:
        mutable clock::time_point _start;
    };

    class timer
    {
    public:
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
            return sw.get_start_time() + _delta <= sw.now();
        }

        // Force time expiration
        void signal() 
        {
            sw.reset(sw.now() - (_delta + std::chrono::nanoseconds(100)));
        }
    protected:
        clock::duration _delta;
        stopwatch sw;
    };

    class periodic_timer
    {
    public:
        using clock = std::chrono::steady_clock;

        periodic_timer(clock::duration delta)
            : t(delta)
        {
        }

        // Allow use of bool operator for checking time expiration and re trigger when time expired
        operator bool() const
        {
            if (t.has_expired())
            {
                t.start();
                return true;
            }
            return false;
        }

        // Force time expiration
        void signal() 
        {
            t.signal();
        }
    protected:
        timer t;
    };
}
