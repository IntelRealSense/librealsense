// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.
#pragma once

#include <fastrtps/types/TypesBase.h>
#include "dds-exceptions.h"

#include <rsutils/easylogging/easyloggingpp.h>
#include <rsutils/string/slice.h>

// FastDDS, in most case, is using two error conventions:
// 1. ReturnCode_t api_call(...)   -> return ReturnCode_t != RETCODE_OK
// 2. <class_name> * api_call(...) -> return nullptr

// Here we covered this 2 options with our own functions which log the error and raise the appropriate exception
// The static function are made to distinguish between the 2 calling formats
namespace realdds {


std::string get_dds_error( eprosima::fastrtps::types::ReturnCode_t );

template< typename T >
inline std::string get_dds_error( T * address )
{
    (void)address;
    return "returned null";
}

// Given a topic-root, return a debug name to use for debug
rsutils::string::slice device_name_from_root( std::string const & topic_root );


}  // namespace realdds

#define DDS_API_CALL_THROW( desc, r )                                                              \
    DDS_THROW( runtime_error, "DDS API CALL '" desc "' " + realdds::get_dds_error( r ) )

#define DDS_API_CALL( func )                                                                       \
    [&]() {                                                                                        \
        auto r = func;                                                                             \
        if( ! r )                                                                                  \
        {                                                                                          \
            DDS_API_CALL_THROW( #func, r );                                                        \
        }                                                                                          \
        return r;                                                                                  \
    }()

// We assume FastDDS API does not throw
#define DDS_API_CALL_NO_THROW( func )                                                              \
    [&]() {                                                                                        \
        auto r = func;                                                                             \
        if( ! r )                                                                                  \
        {                                                                                          \
            LOG_ERROR( "DDS API CALL '" << #func << "' " << realdds::get_dds_error( r ) );         \
        }                                                                                          \
        return r;                                                                                  \
    }()
