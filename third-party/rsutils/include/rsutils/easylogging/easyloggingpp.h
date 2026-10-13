// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021-4 RealSense, Inc. All Rights Reserved.
#pragma once

// When including this file outside LibRealSense you also need to:
// 1. Compile the easylogging++.cc file
// 2. With static linkage, ELPP is initialized by librealsense, so doing it here will
//    create errors. When we're using the shared .so/.dll, the two are separate and we have
//    to initialize ours if we want to use the APIs!
// Use this code snippet:
// 
//#include <easylogging++.h>
//#ifdef BUILD_SHARED_LIBS
//INITIALIZE_EASYLOGGINGPP
//#endif

// you can use 'include(easyloggingpp.cmake)' for setting required includes and sources variables,
// and then use '${ELPP_SOURCES}' and '${ELPP_INCLUDES}' CMake variable to add to your target

#if BUILD_EASYLOGGINGPP
#include <third-party/easyloggingpp/src/easylogging++.h>


#define LIBREALSENSE_ELPP_ID "librealsense"

#include <atomic>
#include <chrono>
#include <thread>

namespace rsutils {
namespace easylogging {
// Process-exit guard for the log macros below.
//
// The ELPP storage is a static object, destroyed during static destruction. A librealsense thread (device
// watcher, dispatcher) can still be running at that point when the application keeps a global rs2 object
// (pipeline, context, device) alive past main(): the context, and the threads with it, are then torn down
// after the logger, and a log call from such a thread used to be a use-after-free. The sentinel defined by
// RSUTILS_INITIALIZE_EASYLOGGINGPP - in the same translation unit as the storage, right after it, so it is
// destroyed right before it - flips shutting_down and waits for the calls already in flight; every later
// call is a no-op. Both globals are trivially destructible, so reading them after that is well-defined.
extern std::atomic< bool > shutting_down;
extern std::atomic< int > calls_in_flight;

struct log_call_guard
{
    bool const ok;
    log_call_guard() : ok( ( ++calls_in_flight, ! shutting_down.load() ) ) {}
    ~log_call_guard() { --calls_in_flight; }
};
}  // namespace easylogging
}  // namespace rsutils

// Use in place of INITIALIZE_EASYLOGGINGPP (see log_call_guard above)
#define RSUTILS_INITIALIZE_EASYLOGGINGPP                                                                                \
    INITIALIZE_EASYLOGGINGPP                                                                                           \
    namespace rsutils {                                                                                                \
    namespace easylogging {                                                                                            \
    std::atomic< bool > shutting_down{ false };                                                                        \
    std::atomic< int > calls_in_flight{ 0 };                                                                           \
    static struct shutdown_sentinel                                                                                    \
    {                                                                                                                  \
        ~shutdown_sentinel()                                                                                           \
        {                                                                                                              \
            shutting_down = true;                                                                                      \
            for( int i = 0; i < 1000 && calls_in_flight > 0; ++i )                                                     \
                std::this_thread::sleep_for( std::chrono::milliseconds( 1 ) );                                         \
        }                                                                                                              \
    } const shutdown_sentinel_instance;                                                                                \
    }                                                                                                                  \
    }


#ifdef __ANDROID__  
#include <android/log.h>
#include <sstream>

#define ANDROID_LOG_TAG "librs"

#define LOG_INFO(...)   do { std::ostringstream ss; ss << __VA_ARGS__; __android_log_write( ANDROID_LOG_INFO, ANDROID_LOG_TAG, ss.str().c_str() ); } while(false)
#define LOG_WARNING(...)   do { std::ostringstream ss; ss << __VA_ARGS__; __android_log_write( ANDROID_LOG_WARN, ANDROID_LOG_TAG, ss.str().c_str() ); } while(false)
#define LOG_ERROR(...)   do { std::ostringstream ss; ss << __VA_ARGS__; __android_log_write( ANDROID_LOG_ERROR, ANDROID_LOG_TAG, ss.str().c_str() ); } while(false)
#define LOG_FATAL(...)   do { std::ostringstream ss; ss << __VA_ARGS__; __android_log_write( ANDROID_LOG_ERROR, ANDROID_LOG_TAG, ss.str().c_str() ); } while(false)
#ifdef NDEBUG
#define LOG_DEBUG(...)
#else
#define LOG_DEBUG(...)   do { std::ostringstream ss; ss << __VA_ARGS__; __android_log_write( ANDROID_LOG_DEBUG, ANDROID_LOG_TAG, ss.str().c_str() ); } while(false)
#endif

#define LOG_DEBUG_STR(STR)    LOG_DEBUG( STR )
#define LOG_INFO_STR(STR)     LOG_INFO( STR )
#define LOG_WARNING_STR(STR)  LOG_WARNING( STR )
#define LOG_ERROR_STR(STR)    LOG_ERROR( STR )
#define LOG_FATAL_STR(STR)    LOG_FATAL( STR )

#else //__ANDROID__  

// Direct log to ELPP, without conversion to string first; use this as an optimization, if you have simple string output
//
// We've seen cases where this fails in U22, causing weird effects with custom overloads/types (e.g., json)
//
namespace rsutils {
// log_to_console() et al. reconfigure the logger (replacing TypedConfigurations) under its lock, from any thread, at
// any time; check enabled() under that same lock too, or risk dereferencing a TypedConfigurations mid-delete.
inline bool elpp_enabled( el::Logger * logger__, el::Level level )
{
    if( ! logger__ )
        return false;
    el::base::threading::ScopedLock lock__( logger__->lock() );
    return logger__->typedConfigurations() && logger__->enabled( level );
}
}  // namespace rsutils

#define LIBRS_LOG_STR_( LEVEL, STR )                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        rsutils::easylogging::log_call_guard guard__;                                                                  \
        if( ! guard__.ok )                                                                                             \
            break;                                                                                                     \
        auto logger__ = el::Loggers::getLogger( rsutils::g_librealsense_elpp_id );                                     \
        if( rsutils::elpp_enabled( logger__, el::Level::LEVEL ) )                                                      \
        {                                                                                                              \
            el::base::Writer( el::Level::LEVEL, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog )   \
                    .construct( logger__ )                                                                             \
                << STR;                                                                                                \
        }                                                                                                              \
    }                                                                                                                  \
    while( false )

// Same, but first convert to string using usual mechanisms
#define LIBRS_LOG_( LEVEL, ... )                                                                                       \
    do                                                                                                                 \
    {                                                                                                                  \
        rsutils::easylogging::log_call_guard guard__;                                                                  \
        if( ! guard__.ok )                                                                                             \
            break;                                                                                                     \
        auto logger__ = el::Loggers::getLogger( rsutils::g_librealsense_elpp_id );                                     \
        if( rsutils::elpp_enabled( logger__, el::Level::LEVEL ) )                                                      \
        {                                                                                                              \
            std::ostringstream os__;                                                                                   \
            os__ << __VA_ARGS__;                                                                                       \
            el::base::Writer( el::Level::LEVEL, __FILE__, __LINE__, ELPP_FUNC, el::base::DispatchAction::NormalLog )   \
                    .construct( logger__ )                                                                             \
                << os__.str();                                                                                         \
        }                                                                                                              \
    }                                                                                                                  \
    while( false )

#define LOG_DEBUG(...)    LIBRS_LOG_( Debug,    __VA_ARGS__ )
#define LOG_INFO(...)     LIBRS_LOG_( Info,     __VA_ARGS__ )
#define LOG_WARNING(...)  LIBRS_LOG_( Warning,  __VA_ARGS__ )
#define LOG_ERROR(...)    LIBRS_LOG_( Error,    __VA_ARGS__ )
#define LOG_FATAL(...)    LIBRS_LOG_( Fatal,    __VA_ARGS__ )

#define LOG_DEBUG_STR(STR)    LIBRS_LOG_STR_( Debug,    STR )
#define LOG_INFO_STR(STR)     LIBRS_LOG_STR_( Info,     STR )
#define LOG_WARNING_STR(STR)  LIBRS_LOG_STR_( Warning,  STR )
#define LOG_ERROR_STR(STR)    LIBRS_LOG_STR_( Error,    STR )
#define LOG_FATAL_STR(STR)    LIBRS_LOG_STR_( Fatal,    STR )

namespace rsutils {


// This is a caching of LIBREALSENSE_ELPP_ID in a string, as a performance optimization
extern std::string const g_librealsense_elpp_id;


// Configure the same logger as librealsense (by default), to disable/enable debug output
void configure_elpp_logger( bool enable_debug = false,
                            std::string const & nested_indent = "",
                            std::string const & logger_id = LIBREALSENSE_ELPP_ID );


}  // namespace rsutils


#endif // __ANDROID__  


#else // BUILD_EASYLOGGINGPP


#define LOG_DEBUG(...)   do { ; } while(false)
#define LOG_INFO(...)    do { ; } while(false)
#define LOG_WARNING(...) do { ; } while(false)
#define LOG_ERROR(...)   do { ; } while(false)
#define LOG_FATAL(...)   do { ; } while(false)

#define LOG_DEBUG_STR(STR)    do { ; } while(false)
#define LOG_INFO_STR(STR)     do { ; } while(false)
#define LOG_WARNING_STR(STR)  do { ; } while(false)
#define LOG_ERROR_STR(STR)    do { ; } while(false)
#define LOG_FATAL_STR(STR)    do { ; } while(false)


#endif // BUILD_EASYLOGGINGPP
