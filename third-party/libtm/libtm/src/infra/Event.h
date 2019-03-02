// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include "Log.h"
#include <mutex>

namespace perc 
{
    class Event
    {
    public:
        Event();

        inline Handle handle() { return mEvent; }

        int signal();

        int reset();

        ~Event();

    private:
        Handle mEvent;
        Handle mTrigger;
        std::mutex mMutex;
        bool mTriggered = false;

        void operator= (const Event &) = delete;
        Event(const Event &) = delete;
    };

} // namespace perc
