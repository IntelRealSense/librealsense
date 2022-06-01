// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <fastrtps/types/TypesBase.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>

// FastDDS API in most cases is build from 2 formats:
// 1. ReturnCode_t api_call(...)
// 2. <class_name> * api_call(...)

// Here we covered this 2 options with are own functions which log the error and raise the appropriate exception
// The static function are made to distinguish between the 2 calling formats
namespace librealsense {
namespace dds {
static std::string get_dds_error( eprosima::fastrtps::types::ReturnCode_t ret_code )
{
    std::stringstream s;
    s << " Err:" << ret_code();
    return s.str();
}

template < typename T > 
static std::string get_dds_error( T ret_code )
{
    return "";
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
