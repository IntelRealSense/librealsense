// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include "TrackingCommon.h"
#include "Log.h"

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

        void operator= (const Event &) = delete;
        Event(const Event &) = delete;
    };

} // namespace perc
