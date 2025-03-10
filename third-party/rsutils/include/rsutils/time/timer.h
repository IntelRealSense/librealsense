// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2020-5 Intel Corporation. All Rights Reserved.
#pragma once

#include "stopwatch.h"

namespace rsutils {
namespace time {


// A timer counting backwards in time (vs forwards in the `stopwatch` class)
// It supply basic timer API, start, has_expired..
class timer
{
public:
    timer( clock::duration timeout )
        : _delta( timeout )
        , sw()
    {
    }

    // Start timer
    void start() { sw.reset(); }

    void reset( clock::duration new_timeout )
    {
        _delta = new_timeout;
        start();
    }

    clock::time_point expiration_time() const { return sw.get_start() + _delta; }

    // Check if timer time expired
    bool has_expired() const { return clock::now() >= expiration_time(); }

    // Force time expiration
    void set_expired() { sw.reset( clock::now() - ( _delta + std::chrono::nanoseconds( 100 ) ) ); }

    clock::duration time_left() const { return expiration_time() - clock::now(); }

protected:
    clock::duration _delta;
    stopwatch sw;
};


}  // namespace time
}  // namespace rsutils
