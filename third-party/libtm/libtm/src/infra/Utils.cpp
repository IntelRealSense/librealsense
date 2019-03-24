// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#define LOG_TAG "Utils"
#include "Utils.h"
#include <chrono>
#include <algorithm>

#ifndef _WIN32
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <string.h>
#include <unistd.h>
#include <sys/resource.h>
#include <math.h>
#ifdef __linux__
#define gettid() syscall(SYS_gettid)
#else
#define gettid() 0L
#endif
#else
#include <windows.h>
#include <tchar.h>
#endif

#include "Utils.h"

#ifdef __APPLE__

#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)

#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)

#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)

#endif

nsecs_t systemTime()
{
    auto start = std::chrono::system_clock::now();
    std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> time_point_ns(start);
    return time_point_ns.time_since_epoch().count();
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
#if defined(__linux__) || defined(__APPLE__)
    return htobe64(val);
#else
    return _byteswap_uint64(val);
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

