// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2017 Intel Corporation. All Rights Reserved.

#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

// Log priority values, in ascending priority order.
// Note: This definition is using the same priority order as android log subsystem.
typedef enum LogPriority {
    LOG_UNKNOWN = 0,
    LOG_DEFAULT,
    LOG_VERBOSE,
    LOG_DEBUG,
    LOG_TRACE,
    LOG_INFO,
    LOG_WARN,
    LOG_ERROR,
    LOG_FATAL,
    LOG_SILENT,
#define LOG_PRI2MASK(_pri)  (1 << _pri)
} LogPriority;

enum LogSource {
    LogSourceHost = 0x0000, /**< Host Log Configuration */
    LogSourceFW = 0x0001,   /**< FW Log Configuration   */
    LogSourceMax = 0x0002,
};

// Normally we strip LOGV (VERBOSE messages) from release builds.
// You can modify this (for example with "#define LOG_NDEBUG 0" at the top of your source file) to change that behavior.
#ifndef NDEBUG
#define NDEBUG
#endif

#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

#ifndef LOG_TAG
#define LOG_TAG NULL
#endif

// Macro to send a verbose log message using the current LOG_TAG.
#ifndef LOGV
#if LOG_NDEBUG
#define LOGV(...)       ((void)0)
#else
#define LOGV(...)       ((void)LOG(NULL, LOG_VERBOSE, LOG_TAG, __LINE__, __VA_ARGS__))
#endif
#endif

#ifndef DEVICELOGV
#if LOG_NDEBUG
#define DEVICELOGV(...) ((void)0)
#else
#define DEVICELOGV(...) ((void)LOG(this, LOG_VERBOSE, LOG_TAG, __LINE__, __VA_ARGS__))
#endif
#endif

// Macros to send a priority log message using the current LOG_TAG.
#ifndef LOGD
#define LOGD(...)       ((void)LOG(NULL, LOG_DEBUG, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOGT
#define LOGT(...)       ((void)LOG(NULL, LOG_TRACE, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOGI
#define LOGI(...)       ((void)LOG(NULL, LOG_INFO, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOGW
#define LOGW(...)       ((void)LOG(NULL, LOG_WARN, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOGE
#define LOGE(...)       ((void)LOG(NULL, LOG_ERROR, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOGF
#define LOGF(...)       ((void)LOG(NULL, LOG_FATAL, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGD
#define DEVICELOGD(...)       ((void)LOG(this, LOG_DEBUG, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGT
#define DEVICELOGT(...)       ((void)LOG(this, LOG_TRACE, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGI
#define DEVICELOGI(...)       ((void)LOG(this, LOG_INFO, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGW
#define DEVICELOGW(...)       ((void)LOG(this, LOG_WARN, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGE
#define DEVICELOGE(...)       ((void)LOG(this, LOG_ERROR, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef DEVICELOGF
#define DEVICELOGF(...)       ((void)LOG(this, LOG_FATAL, LOG_TAG, __LINE__, __VA_ARGS__))
#endif

#ifndef LOG
#define LOG(_deviceId, _prio, _tag, _line, ...) \
    __perc_Log_print(_deviceId, _prio, _tag, _line, __VA_ARGS__)
#endif

#define ASSERT          assert
#define UNUSED(_var)    (void)(_var)

// platform depended print declaration
void __perc_Log_write(int prio, const char *tag, const char *text);
void __perc_Log_print(void* deviceId, int prio, const char *tag, int line, const char *fmt, ...);
int __perc_Log_print_header(char * buf, int bufSize, int prio, const char *tag, const char* deviceId);
void __perc_Log_Save(void* deviceId, int prio, int payloadLen, char* payload);
void __perc_Log_Set_Configuration(uint8_t source, uint8_t outputMode, uint8_t verbosity, uint8_t rolloverMode);
void __perc_Log_Get_Configuration(uint8_t source, uint8_t* outputMode, uint8_t* verbosityMask, uint8_t* rolloverMode);
void __perc_Log_Get_Log(void* log);

