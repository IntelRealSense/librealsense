// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Utils"
#include "Utils.h"
#include <chrono>
#include <algorithm>
#include <math.h>

#ifdef __linux__
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#define gettid() syscall(SYS_gettid)
#else
#include <windows.h>
#include <tchar.h>
#endif

nsecs_t systemTime()
{
#ifdef _WIN32
    /*
    auto start = std::chrono::high_resolution_clock::now();
    std::chrono::time_point<std::chrono::high_resolution_clock, std::chrono::nanoseconds> time_point_ns(start);
    return time_point_ns.time_since_epoch().count();*/
    LARGE_INTEGER StartingTime = { 0 };
    LARGE_INTEGER Frequency = { 0 };

    QueryPerformanceFrequency(&Frequency);
    QueryPerformanceCounter(&StartingTime);

    StartingTime.QuadPart *= 1000000LL;   // convert to microseconds
    StartingTime.QuadPart /= Frequency.QuadPart;
    StartingTime.QuadPart *= 1000;       // Convert to nano seconds

    return StartingTime.QuadPart;

#else
    struct timespec ts;
    ts.tv_sec = ts.tv_nsec = 0;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ((nsecs_t)(ts.tv_sec)) * 1000000000LL + ts.tv_nsec;
#endif
}


HostLocalTime getLocalTime()
{
    HostLocalTime localTime;

#ifdef _WIN32
    SYSTEMTIME lt;
    GetLocalTime(&lt);

    localTime.year = lt.wYear;
    localTime.month = lt.wMonth;
    localTime.dayOfWeek = lt.wDayOfWeek;
    localTime.day = lt.wDay;
    localTime.hour = lt.wHour;
    localTime.minute = lt.wMinute;
    localTime.second = lt.wSecond;
    localTime.milliseconds = lt.wMilliseconds;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);

    // Round to nearest millisec
    int millisec = lrint(tv.tv_usec / 1000.0);
    if (millisec >= 1000)
    {
        millisec -= 1000;
        tv.tv_sec++;
    }

    struct tm* tm_info = localtime(&tv.tv_sec); // Transform time to ASCII

    localTime.year = tm_info->tm_year;
    localTime.month = tm_info->tm_mon;
    localTime.dayOfWeek = tm_info->tm_wday;
    localTime.day = tm_info->tm_mday;
    localTime.hour = tm_info->tm_hour;
    localTime.minute = tm_info->tm_min;
    localTime.second = tm_info->tm_sec;
    localTime.milliseconds = millisec;
#endif

    return localTime;
}


uint64_t bytesSwap(uint64_t val)
{
#ifdef _WIN32
    return _byteswap_uint64(val);
#else
    return htobe64(val);
#endif
}

uint32_t getThreadId(void)
{
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return gettid();
#endif
}


void perc::copy(void* dst, void const* src, size_t size)
{
#ifdef _WIN32
    memcpy_s(dst, size, src, size);
#else
    auto from = reinterpret_cast<uint8_t const*>(src);
    std::copy(from, from + size, reinterpret_cast<uint8_t*>(dst));
#endif
}


size_t perc::stringLength(const char* string, size_t maxSize)
{
#ifdef _WIN32
    return strnlen_s(string, maxSize);
#else
    return strnlen(string, maxSize);
#endif
}

bool setProcessPriorityToRealtime()
{
#ifdef _WIN32
    HANDLE pid = GetCurrentProcess();
    if (!SetPriorityClass(pid, REALTIME_PRIORITY_CLASS))
    {
        LOGE("Error: Failed to set process priority to runtime");
        return false;
    }
    // Display priority class
    DWORD dwPriClass = GetPriorityClass(pid);
    LOGD("Setting process priority to 0x%X", dwPriClass);
#else
    const int MAX_NICENESS = -20;

    id_t pid = getpid();
    int ret = setpriority(PRIO_PROCESS, pid, MAX_NICENESS);
    if (ret == -1)
    {
        LOGE("Error: Failed to set process priority - to set priority rerun with sudo\n");
        return true;
    }
    // Display priority class
    ret = getpriority(PRIO_PROCESS, pid);
    LOGD("Setting process priority to 0x%X", ret);
#endif

    return true;
}

