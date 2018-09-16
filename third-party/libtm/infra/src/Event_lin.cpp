/*******************************************************************************
INTEL CORPORATION PROPRIETARY INFORMATION
Copyright(c) 2017 Intel Corporation. All Rights Reserved.
*******************************************************************************/

#include "Event.h"
#include <unistd.h>
# include <sys/eventfd.h>

perc::Event::Event()
{
    // LINUX: NONBLOCK
    mEvent = ::eventfd(0, EFD_NONBLOCK);

    ASSERT(mEvent != ILLEGAL_HANDLE);
}

perc::Event::~Event()
{
    ::close(mEvent);
}

int perc::Event::reset()
{
    ASSERT(mEvent != ILLEGAL_HANDLE);

    eventfd_t cnt;
    return !::eventfd_read(mEvent, &cnt);
}

int perc::Event::signal() {
    ASSERT(mEvent != ILLEGAL_HANDLE);

    return !::eventfd_write(mEvent, 1);
}