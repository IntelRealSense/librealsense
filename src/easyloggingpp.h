// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2021 Intel Corporation. All Rights Reserved.

#pragma once

#if BUILD_EASYLOGGINGPP
#include "../third-party/easyloggingpp/src/easylogging++.h"


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

#define LOG_DEBUG(...)   do { CLOG(DEBUG   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_INFO(...)    do { CLOG(INFO    ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_WARNING(...) do { CLOG(WARNING ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_ERROR(...)   do { CLOG(ERROR   ,"librealsense") << __VA_ARGS__; } while(false)
#define LOG_FATAL(...)   do { CLOG(FATAL   ,"librealsense") << __VA_ARGS__; } while(false)

#endif // __ANDROID__  


#else // BUILD_EASYLOGGINGPP


#define LOG_DEBUG(...)   do { ; } while(false)
#define LOG_INFO(...)    do { ; } while(false)
#define LOG_WARNING(...) do { ; } while(false)
#define LOG_ERROR(...)   do { ; } while(false)
#define LOG_FATAL(...)   do { ; } while(false)


#endif // BUILD_EASYLOGGINGPP
