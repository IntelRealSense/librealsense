// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once

#include <stdint.h>
#include "Log.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <time.h>
#define INFINITE (-1)
#endif

class HostLocalTime {
public:
    HostLocalTime() : year(0), month(0), dayOfWeek(0), day(0), hour(0), minute(0), second(0), milliseconds(0) {}
    uint16_t year;
    uint16_t month;
    uint16_t dayOfWeek;
    uint16_t day;
    uint16_t hour;
    uint16_t minute;
    uint16_t second;
    uint16_t milliseconds;
};

HostLocalTime getLocalTime();

#ifdef __cplusplus
extern "C" {
#endif
    typedef int64_t nsecs_t;

    inline nsecs_t s2ns(nsecs_t sec)
    {
        return (sec < 0) ? INFINITE : sec * 1000000000;
    }

    inline nsecs_t ms2ns(nsecs_t ms)
    {
        return (ms < 0) ? INFINITE : ms * 1000000;
    }

    inline int ns2s(nsecs_t ns)
    {
        return (ns < 0) ? INFINITE : (int)(ns / 1000000000);
    }

    inline int ns2ms(nsecs_t ns)
    {
        return (ns < 0) ? INFINITE : (int)(ns / 1000000);
    }

    nsecs_t systemTime();

    uint64_t bytesSwap(uint64_t val);

    uint32_t getThreadId(void);

    inline int toMillisecondTimeoutDelay(nsecs_t referenceTime, nsecs_t timeoutTime)
    {
        int timeoutDelayMsec = 0;

        if (timeoutTime > referenceTime)
        {
            timeoutDelayMsec = (int)(((uint64_t)(timeoutTime - referenceTime) + 999999LL) / 1000000LL);
        }

        return timeoutDelayMsec;
    }

    bool setProcessPriorityToRealtime();

    namespace perc
    {
        void copy(void* dst, void const* src, size_t size);
        size_t stringLength(const char* string, size_t maxSize);
    }

#ifdef __cplusplus
}
#endif