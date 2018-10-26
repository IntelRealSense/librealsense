// License: Apache 2.0. See LICENSE file in root directory.
//
// a common/simple log mechanism for troubleshooting Linux kernel issue, Android source code issue, Linux source code issue

#ifndef __CDE_LOG__
#define __CDE_LOG__

#ifndef __KERNEL__
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h> 
#if  defined(ANDROID) || defined(__ANDROID__)
#include <android/log.h>
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


#ifndef ANDROID
/*
 * Android log priority values, in ascending priority order.
 */
typedef enum android_LogPriority {
    ANDROID_LOG_UNKNOWN = 0,
    ANDROID_LOG_DEFAULT,    /* only for SetMinPriority() */
    ANDROID_LOG_VERBOSE,
    ANDROID_LOG_DEBUG,
    ANDROID_LOG_INFO,
    ANDROID_LOG_WARN,
    ANDROID_LOG_ERROR,
    ANDROID_LOG_FATAL,
    ANDROID_LOG_SILENT,     /* only for SetMinPriority(); must be last */
} android_LogPriority;
#endif


#ifndef LOG_TAG
    #define LOG_TAG NULL
#endif


#ifndef __IRSA_DEBUG__
        #define LOG_PRI(file, func, line, priority, tag, ...) ((void)0)

#else

        void  LOG_PRI_IMPL(const char *file, const char *func, unsigned int line,  int priority, const char *tag, const char *format,...);

        #define LOG_PRI(file, func, line, priority, tag, ... ) \
                LOG_PRI_IMPL(file, func, line, priority, tag, __VA_ARGS__)

#endif

//begin common macro 
#ifndef LOGV
#define LOGV(...) LOG_PRI(__FILE__, __FUNCTION__, __LINE__, ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGD
#define LOGD(...) LOG_PRI(__FILE__, __FUNCTION__, __LINE__, ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGI
#define LOGI(...) LOG_PRI(__FILE__, __FUNCTION__, __LINE__, ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGW
#define LOGW(...) LOG_PRI(__FILE__, __FUNCTION__, __LINE__, ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#endif

#ifndef LOGE
#define LOGE(...) LOG_PRI(__FILE__, __FUNCTION__, __LINE__, ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#endif
//end common macro 


//borrow from AOSP
#if  defined(ANDROID) || defined(__ANDROID__)
#define CONDITION(cond)                         (__builtin_expect((cond) != 0, 0))
#define __android_second(dummy, second, ...)    second
#define __android_rest(first, ...)              , ## __VA_ARGS__

#define android_printAssert(cond, tag, fmt...)  __android_log_assert(cond, tag, __android_second(0, ## fmt, NULL) __android_rest(fmt))


#ifndef LOG_ALWAYS_FATAL_IF
    #ifndef __IRSA_DEBUG__
        #define LOG_ALWAYS_FATAL_IF(cond, ...) \
            ( (CONDITION(cond)) \
            ? ((void)LOGE(__VA_ARGS__)) \
            : (void)0 )
    #else
        #define LOG_ALWAYS_FATAL_IF(cond, ...) \
            ( (CONDITION(cond)) \
            ? ((void)android_printAssert(#cond, LOG_TAG, ## __VA_ARGS__)) \
            : (void)0 )
    #endif
#endif

#ifndef LOG_ALWAYS_FATAL
    #ifndef __IRSA_DEBUG__
        #define LOG_ALWAYS_FATAL(...) \
            ((void)LOGE(__VA_ARGS__))
    #else
        #define LOG_ALWAYS_FATAL(...) \
        ( ((void)android_printAssert(NULL, LOG_TAG, ## __VA_ARGS__)) )
    #endif
#endif


#ifndef __IRSA_DEBUG__
    #ifndef LOG_FATAL_IF
        #define LOG_FATAL_IF(cond, ...) ((void)0)
    #endif

    #ifndef LOG_FATAL
        #define LOG_FATAL(...) ((void)0)
    #endif

#else

    #ifndef LOG_FATAL_IF
        #define LOG_FATAL_IF(cond, ...) LOG_ALWAYS_FATAL_IF(cond, ## __VA_ARGS__)
    #endif

    #ifndef LOG_FATAL
        #define LOG_FATAL(...) LOG_ALWAYS_FATAL(__VA_ARGS__)
    #endif

#endif

#ifndef LOG_ASSERT
#define LOG_ASSERT(cond, ...) LOG_FATAL_IF(!(cond), ## __VA_ARGS__)
#endif

#define LITERAL_TO_STRING_INTERNAL(x)    #x
#define LITERAL_TO_STRING(x) LITERAL_TO_STRING_INTERNAL(x)

#ifndef __IRSA_DEBUG__
    #define CHECK(condition) (condition)

#else

    #define CHECK(condition)                            \
        LOG_ALWAYS_FATAL_IF(                            \
            !(condition),                               \
            "%s",                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)    \
            " CHECK(" #condition ") failed.")
#endif


#ifndef __IRSA_DEBUG__
#define TRESPASS() ((void)0)
#else
#define TRESPASS() \
        LOG_ALWAYS_FATAL(                                       \
            __FILE__ ":" LITERAL_TO_STRING(__LINE__)            \
                " Should not be here.");
#endif

#endif // #if  defined(ANDROID) || defined(__ANDROID__)

#ifdef __cplusplus
}
#endif

#endif
