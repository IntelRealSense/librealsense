// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <thread>
#include <mutex>
#include <unordered_map>
#include <memory>
#include "TrackingCommon.h"

namespace perc {

    class Poller
    {
    public:

        struct event
        {
            Handle handle;
            unsigned long mask;
            void *act;
            event() :
                handle(ILLEGAL_HANDLE), mask(Poller::NULL_MASK), act(0) {}
            event(Handle h, unsigned long m, void *a) :
                handle(h), mask(m), act(a) {}
        };

        Poller();
        ~Poller();
        bool add(const Poller::event &);
        bool remove(Handle);

        // POLLING LOOP
        //
        // Returns:
        //      >0 - the total number of events
        //      0  - if the <timeout> elapsed without dispatching any handle
        //      -1 - if an error occurs
        int poll(Poller::event &, unsigned long timeoutMs);

    public:

        static const int NULL_MASK;
        static const int READ_MASK;
        static const int WRITE_MASK;
        static const int EXCEPT_MASK;
        static const int ALL_MASK;


    private:

        struct CheshireCat;
        std::unique_ptr<CheshireCat> mData;

        // Prevent assignment and initialization.
        void operator= (const Poller &) = delete;
        Poller(const Poller &) = delete;
    };

} // namespace perc
