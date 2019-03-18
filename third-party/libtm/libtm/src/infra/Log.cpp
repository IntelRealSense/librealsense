// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#include "Log.h"
#include "Utils.h"
#include <stdbool.h>
#include <stdint.h>
#include <mutex>
#include <atomic>
#include <math.h>
#include "../include/TrackingData.h"

#ifdef _WIN32
#include <Windows.h>
#include <stdio.h>
#include <time.h>
#endif

#ifndef _WIN32
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/syscall.h>
#ifdef __linux__
#define gettid() syscall(SYS_gettid)
#else
#define gettid() 0L
#endif
#endif

using namespace perc;

/* Define TIME_OF_DAY_LOG_HEADER to enable time of day (HH:MM:SS.mmm) log */
/* Undefine TIME_OF_DAY_LOG_HEADER to enable epoch time log               */
#define TIME_OF_DAY_LOG_HEADER

#define MAX_LOG_CONTAINERS 2

// Default priority mask - ALL enabled
static int s_PriorityMask = /*LOG_PRI2MASK(LOG_VERBOSE) | */
LOG_PRI2MASK(LOG_DEBUG) |
LOG_PRI2MASK(LOG_TRACE) |
LOG_PRI2MASK(LOG_INFO) |
LOG_PRI2MASK(LOG_WARN) |
LOG_PRI2MASK(LOG_ERROR);

class AdvancedLog : public TrackingData::Log {
public:
    AdvancedLog() : rolledOver(0) {};

    uint8_t rolledOver;
};


class LogConfiguration {
public:
    uint32_t outputMode;
    uint8_t verbosityMask;
    uint8_t rolloverMode;
} ;

class logManager {
public:
    logManager() : activeContainer(0) 
    {
        loadTimestamp = systemTime();
        configuration[LogSourceHost].verbosityMask = s_PriorityMask;
        configuration[LogSourceHost].rolloverMode = true;
        configuration[LogSourceHost].outputMode = LogOutputModeScreen;
    };

    nsecs_t loadTimestamp;

    std::mutex configurationMutex;
    LogConfiguration configuration[LogSourceMax];

    std::atomic<uint8_t> activeContainer;
    std::mutex logContainerMutex[MAX_LOG_CONTAINERS];
    AdvancedLog logContainer[MAX_LOG_CONTAINERS];
};

logManager gLogManager;

FILE *gStream = NULL;

const char* logPrioritySign[] = { "U" ,/* LOG_UNKNOWN */
"T" ,/* LOG_DEFAULT */
"V" ,/* LOG_VERBOSE */
"D" ,/* LOG_DEBUG   */
"T" ,/* LOG_TRACE   */
"I" ,/* LOG_INFO    */
"W" ,/* LOG_WARN    */
"E" ,/* LOG_ERROR   */
"F" ,/* LOG_FATAL   */
"S" ,/* LOG_SILENT  */ };

const LogVerbosityLevel prio2verbosity[] = {None, None, Verbose, Debug, Trace, Info, Warning, Error, Error, None};

// LOG_FATAL is always enabled by default
bool isPriorityEnabled(int prio)
{
    return (LOG_FATAL == prio || (gLogManager.configuration[LogSourceHost].verbosityMask & LOG_PRI2MASK(prio)));
}

void __perc_Log_write(int prio, const char *tag, const char *text)
{
    if (!isPriorityEnabled(prio))
        return;

    fprintf(stdout, "%s", text);

}

 void __perc_Log_Save(void* deviceId, int prio, const char *tag, int line, int payloadLen, char* payload)
{
    std::lock_guard<std::mutex> guardLogContainer(gLogManager.logContainerMutex[gLogManager.activeContainer]);
    AdvancedLog* logContainer = &gLogManager.logContainer[gLogManager.activeContainer];
    uint32_t entries = logContainer->entries;

    if ((gLogManager.configuration[LogSourceHost].rolloverMode == 0) && (logContainer->rolledOver == 1))
    {
        printf("rolled over - stopped saving prints on container %d, entries = %d...\n", (uint32_t)gLogManager.activeContainer, entries);
        return;
    }

    logContainer->entry[entries].timeStamp = systemTime() - gLogManager.loadTimestamp;

    HostLocalTime localTime = getLocalTime();

    logContainer->entry[entries].localTimeStamp.year = localTime.year;
    logContainer->entry[entries].localTimeStamp.month = localTime.month;
    logContainer->entry[entries].localTimeStamp.dayOfWeek = localTime.dayOfWeek;
    logContainer->entry[entries].localTimeStamp.day = localTime.day;
    logContainer->entry[entries].localTimeStamp.hour = localTime.hour;
    logContainer->entry[entries].localTimeStamp.minute = localTime.minute;
    logContainer->entry[entries].localTimeStamp.second = localTime.second;
    logContainer->entry[entries].localTimeStamp.milliseconds = localTime.milliseconds;

    logContainer->entry[entries].lineNumber = line;

    if (tag != NULL)
    {
        perc::copy(&logContainer->entry[entries].moduleID, tag, MAX_LOG_BUFFER_MODULE_SIZE);
    }

    logContainer->entry[entries].verbosity = prio2verbosity[prio];
    logContainer->entry[entries].deviceID = (uint64_t)deviceId;
    logContainer->entry[entries].threadID = getThreadId();

    snprintf(logContainer->entry[entries].payload, payloadLen + 1, "%s", payload);
    logContainer->entry[entries].payloadSize = payloadLen;

    if (entries == (MAX_LOG_BUFFER_ENTRIES-1))
    {
        printf("entries %d, setting rollover = 1...\n", logContainer->entries);
        logContainer->rolledOver = 1;
    }

    logContainer->entries = (entries + 1) % MAX_LOG_BUFFER_ENTRIES;
    
}


#ifndef _WIN32
int __perc_Log_print_header(char * buf, int bufSize, int prio, const char *tag, const char* deviceId)
{
    struct timeval tv;
    int headerLen;
    static struct timeval loadTime = { 0 };

    gettimeofday(&tv, NULL);

#ifdef TIME_OF_DAY_LOG_HEADER

    // Round to nearest millisec
    int millisec = lrint(tv.tv_usec / 1000.0);
    if (millisec >= 1000)
    {
        millisec -= 1000;
        tv.tv_sec++;
    }

    struct tm* tm_info = localtime(&tv.tv_sec); // Transform time to ASCII
    int timeLen = 8; // Length of adding HH:MM:SS
    strftime(buf, timeLen + 2, "%H:%M:%S", tm_info); // Adding HH:MM:SS/0 to buffer
    headerLen = snprintf(buf + timeLen, bufSize - timeLen, ".%03d [%lu] [%s] %s%s: ", millisec, gettid(), logPrioritySign[prio], tag, deviceId) + timeLen;
    
#else
        if (loadTime.tv_sec == 0)
        {
            loadTime = tv;
        }

        unsigned long currentTime = ((tv.tv_sec * 1000000) + tv.tv_usec) - ((loadTime.tv_sec * 1000000) + loadTime.tv_usec);
        headerLen = snprintf(buf, bufSize, "%014lu [%lu] [%s] %s: ", currentTime, gettid(), logPrioritySign[prio], tag);

#endif //TIME_OF_DAY_LOG_HEADER

    return headerLen;
}
#elif _WIN32
int __perc_Log_print_header(char * buf, int bufSize, int prio, const char *tag, const char* deviceId)
{
    int headerLen = 0;

#ifdef TIME_OF_DAY_LOG_HEADER

    SYSTEMTIME lt;
    GetLocalTime(&lt);

    headerLen = snprintf(buf, bufSize, "%02d:%02d:%02d:%03d [%06lu] [%s] %s%s: ", lt.wHour, lt.wMinute, lt.wSecond, lt.wMilliseconds, GetCurrentThreadId(), logPrioritySign[prio], tag, deviceId);
    
#else
    
    static unsigned long long loadTime = 0;
    FILETIME currentFileTime;
    GetSystemTimeAsFileTime(&currentFileTime);
    unsigned long long currentTime = (((ULONGLONG)currentFileTime.dwHighDateTime << 32) | (ULONGLONG)currentFileTime.dwLowDateTime) / 10000;

    if (loadTime == 0)
    {
        loadTime = currentTime;
    }

    headerLen = snprintf(buf, bufSize, "%011llu [%06lu] [%s] %s: ", currentTime - loadTime, GetCurrentThreadId(), logPrioritySign[prio], tag);

#endif // TIME_OF_DAY_LOG_HEADER

    return headerLen;

}
#endif

void __perc_Log_print(void* deviceId, int prio, const char *tag, int line, const char *fmt, ...)
{
    if (isPriorityEnabled(prio)) {
        uint8_t outputMode;
        uint8_t verbosity;
        uint8_t rolloverMode;
        int headerLen = 0;
        int payloadLen = 0;

        va_list ap;
        va_start(ap, fmt);

        __perc_Log_Get_Configuration(LogSourceHost, &outputMode, &verbosity, &rolloverMode);

        char buf[BUFSIZ * 4];

        if (outputMode == LogOutputModeScreen)
        {
            char deviceBuf[30] = { 0 };
            if (deviceId != NULL)
            {
                short device = ((uintptr_t)deviceId & 0xFFFF);
                snprintf(deviceBuf, sizeof(deviceBuf), "-%04hX", device);
            }

            headerLen = __perc_Log_print_header(buf, sizeof(buf), prio, tag, deviceBuf);

            ASSERT((size_t)headerLen < sizeof(buf));
            payloadLen = vsnprintf(buf + headerLen, sizeof(buf) - headerLen, fmt, ap);
            fprintf(stdout, "%s\n", buf);
        }
        else
        {
            payloadLen = vsnprintf(buf, sizeof(buf), fmt, ap);
            __perc_Log_Save(deviceId, prio, tag, line, payloadLen, buf);
        }

        va_end(ap);
    }
}

void __perc_Log_Set_Configuration(uint8_t source, uint8_t outputMode, uint8_t verbosity, uint8_t rolloverMode)
{
    std::lock_guard<std::mutex> guardLogConfiguration(gLogManager.configurationMutex);

    gLogManager.configuration[source].outputMode = outputMode;
    gLogManager.configuration[source].rolloverMode = rolloverMode;

    uint8_t verbosityMask = 0;
    switch (verbosity)
    {
        case Trace:
            verbosityMask |= LOG_PRI2MASK(LOG_TRACE);
        case Verbose:
            verbosityMask |= LOG_PRI2MASK(LOG_VERBOSE);
        case Debug:
            verbosityMask |= LOG_PRI2MASK(LOG_DEBUG);
        case Warning:
            verbosityMask |= LOG_PRI2MASK(LOG_WARN);
        case Info:
            verbosityMask |= LOG_PRI2MASK(LOG_INFO);
        case Error:
            verbosityMask |= LOG_PRI2MASK(LOG_ERROR);
        case None:
            verbosityMask |= 0;
    }

    gLogManager.configuration[source].verbosityMask = verbosityMask;
}

void __perc_Log_Get_Configuration(uint8_t source, uint8_t* outputMode, uint8_t* verbosity, uint8_t* rolloverMode)
{
    std::lock_guard<std::mutex> guardLogConfiguration(gLogManager.configurationMutex);

    *outputMode = gLogManager.configuration[source].outputMode;
    *verbosity = gLogManager.configuration[source].verbosityMask;
    *rolloverMode = gLogManager.configuration[source].rolloverMode;
}


void __perc_Log_Get_Log(void* log)
{
    TrackingData::Log* logOutput = (TrackingData::Log*)log;
    uint32_t entryIndex = 0;

    uint8_t activeContainer = gLogManager.activeContainer;

    gLogManager.activeContainer ^= 1;

    {
        std::lock_guard<std::mutex> guardLogContainer(gLogManager.logContainerMutex[activeContainer]);
        AdvancedLog* logContainer = &gLogManager.logContainer[activeContainer];

        if (logContainer->rolledOver == 1)
        {
            for (uint32_t i = logContainer->entries; i < MAX_LOG_BUFFER_ENTRIES; i++, entryIndex++)
            {
                logOutput->entry[entryIndex] = logContainer->entry[i];
            }

            logOutput->entries = MAX_LOG_BUFFER_ENTRIES;
        }
        else
        {
            logOutput->entries = logContainer->entries;
        }

        for (uint32_t i = 0; i < logContainer->entries; i++, entryIndex++)
        {
            logOutput->entry[entryIndex] = logContainer->entry[i];
        }
        
        logOutput->maxEntries =  MAX_LOG_BUFFER_ENTRIES;

        logContainer->entries = 0;
        logContainer->rolledOver = 0;

        //printf("got log on container %d - %d entries (new container %d)\n", activeContainer, logOutput->entries, (uint32_t)gLogManager.activeContainer);
    }
}
