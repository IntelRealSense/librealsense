// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

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

#else //__ANDROID__  

#define LOG_DEBUG(...)   do { CLOG(DEBUG   , LIBREALSENSE_ELPP_ID) << __VA_ARGS__; } while(false)
#define LOG_INFO(...)    do { CLOG(INFO    , LIBREALSENSE_ELPP_ID) << __VA_ARGS__; } while(false)
#define LOG_WARNING(...) do { CLOG(WARNING , LIBREALSENSE_ELPP_ID) << __VA_ARGS__; } while(false)
#define LOG_ERROR(...)   do { CLOG(ERROR   , LIBREALSENSE_ELPP_ID) << __VA_ARGS__; } while(false)
#define LOG_FATAL(...)   do { CLOG(FATAL   , LIBREALSENSE_ELPP_ID) << __VA_ARGS__; } while(false)

namespace rsutils {


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


#endif // BUILD_EASYLOGGINGPP
