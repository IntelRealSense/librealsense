// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include "stopwatch.h"

namespace rsutils
{
    namespace time
    {
        // A timer counting backwards in time (vs forwards in the `stopwatch` class)
        // It supply basic timer API, start, has_expired..
        class timer
        {
        public:

            timer(clock::duration timeout) : _delta(timeout), sw()
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
}
