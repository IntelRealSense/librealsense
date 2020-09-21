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

        void reset() const { _start = clock::now(); }

        // Get elapsed milliseconds since timer creation
        double elapsed_ms() const
        {
            return std::chrono::duration_cast<std::chrono::duration<double, std::milli>>(elapsed()).count();
        }

        clock::duration elapsed() const
        {
            return clock::now() - _start;
        }

        clock::time_point now() const
        {
            return clock::now();
        }

    protected:
        mutable clock::time_point _start;
    };

    class timer : public stopwatch
    {
    public:
        timer(clock::duration timeout) : stopwatch()
        {
            _delta = timeout;
        }

        void start() const
        {
            reset();
        }

        bool has_expired() const
        {
            return _start + _delta < now();
        }
    protected:
        clock::duration _delta;
    };

    class periodic_timer : public timer
    {
    public:
        using clock = std::chrono::steady_clock;

        periodic_timer(clock::duration delta)
            : timer(delta)
        {
           
        }

        operator bool() const
        {
            if (this->has_expired())
            {
                this->start();
                return true;
            }
            return false;
        }

        void signal() const
        {
            _start = now() - _delta;
        }

    };
}
