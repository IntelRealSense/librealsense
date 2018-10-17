// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <mutex>
#include <condition_variable>


namespace perc {

class Fence
{
public:
    Fence(){}

    inline void notify() {
        std::unique_lock<std::mutex> l(m);
        fired = true;
        cv.notify_one();
    }

    inline void wait() {
        std::unique_lock<std::mutex> l(m);
        cv.wait(l, [this]{ return fired; });
        fired = false;
    }

private:
    std::mutex m;
    std::condition_variable cv;
    bool fired = false;
    // Prevent assignment and initialization.
    void operator= (const Fence &);
    Fence (const Fence &);
};

} // namespace perc
