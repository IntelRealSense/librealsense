// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include <chrono>
#include "timer.h"

namespace time_utilities
{
    using clock = std::chrono::steady_clock;

    class periodic_timer
    {
    public:

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
        void set_expired()
        {
            t.set_expired();
        }
    protected:
        timer t;
    };
}
