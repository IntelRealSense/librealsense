// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "Event.h"

perc::Event::Event()
{
    // WINDOWS: manual reset event
    mEvent = CreateEvent(
        NULL,               // default security attributes
        TRUE,               // manual-reset event
        FALSE,              // initial state is nonsignaled
        NULL                // Unnamed event
    );

    ASSERT(mEvent != ILLEGAL_HANDLE);
}

perc::Event::~Event()
{
    CloseHandle(mEvent);
}

int perc::Event::reset()
{
    ASSERT(mEvent != ILLEGAL_HANDLE);
    return ResetEvent(mEvent);
}

int perc::Event::signal()
{
    ASSERT(mEvent != ILLEGAL_HANDLE);

    return SetEvent(mEvent);
}