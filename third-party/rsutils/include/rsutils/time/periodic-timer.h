// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020 Intel Corporation. All Rights Reserved.

// Helper classes to keep track of time
#pragma once

#include "timer.h"

namespace rsutils
{
    namespace time
    {
        // A timer class that wakes up every <delta> seconds/minutes/etc. on its bool operator:
        // Usage example:
        //     periodic_timer timer( chrono::seconds( 5 ));
        //     while(1) {
        //         if( timer ) { /* do something every 5 seconds */ }
        //     }
        class periodic_timer
        {
        public:

            periodic_timer(clock::duration delta)
                : _delta(delta)
            {
            }

            // Allow use of bool operator for checking time expiration and re trigger when time expired
            operator bool() const
            {
                if (_delta.has_expired())
                {
                    _delta.start();
                    return true;
                }
                return false;
            }

            // Force time expiration
            void set_expired()
            {
                _delta.set_expired();
            }
        protected:
            timer _delta;
        };
    }
}
