// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "Event.h"
#include <unistd.h>

perc::Event::Event()
{
    int fds[2];
    int ret = pipe(fds);
    ASSERT(ret == 0);
    mEvent = fds[0];
    mTrigger = fds[1];
}

perc::Event::~Event()
{
    close(mEvent);    mEvent    = -1;
    close(mTrigger); mTrigger = -1;
}

int perc::Event::reset()
{
    ASSERT(mEvent != -1);
    {
        std::lock_guard<std::mutex> _(mMutex);
        if (!mTriggered)
            return true;
        mTriggered = false;
    }
    char c[1];
    return read(mEvent,c,sizeof(c)) == 1;
}

int perc::Event::signal()
{
    ASSERT(mEvent != -1);
    {
        std::lock_guard<std::mutex> _(mMutex);
        if (mTriggered)
            return true;
        mTriggered = true;
    }
    char c = 27;
    return write(mTrigger, &c, 1) == 1;
}
