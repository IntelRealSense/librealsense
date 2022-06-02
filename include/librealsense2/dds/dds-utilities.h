// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <fastrtps/types/TypesBase.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <sstream>

// FastDDS, in most case, is using two error conventions:
// 1. ReturnCode_t api_call(...)   -> return ReturnCode_t != RETCODE_OK
// 2. <class_name> * api_call(...) -> return nullptr

// Here we covered this 2 options with our own functions which log the error and raise the appropriate exception
// The static function are made to distinguish between the 2 calling formats
namespace librealsense {
namespace dds {
inline std::string get_dds_error( eprosima::fastrtps::types::ReturnCode_t ret_code )
{
    std::ostringstream s;
    s << " Err:" << ret_code();
    return s.str();
}

template < typename T > 
inline std::string get_dds_error( T *address )
{
    return "returned null";
}
}  // namespace dds
}  // namespace librealsense

#define DDS_API_CALL( func )                                                                       \
    [&]() {                                                                                        \
        auto r = func;                                                                             \
        if( ! r )                                                                                  \
        {                                                                                          \
            LOG_ERROR( "DDS API CALL '" << #func << "'"                                            \
                                        << librealsense::dds::get_dds_error( r ) );                \
            throw std::runtime_error( #func " failed" );                                           \
        }                                                                                          \
        return r;                                                                                  \
    }()

// We assume FastDDS API does not throw
#define DDS_API_CALL_NO_THROW( func )                                                              \
    [&]() {                                                                                        \
        auto r = func;                                                                             \
        if( ! r )                                                                                  \
        {                                                                                          \
            LOG_ERROR( "DDS API CALL '" << #func << "'"                                            \
                                        << librealsense::dds::get_dds_error( r ) );                \
        }                                                                                          \
        return r;                                                                                  \
    }()
