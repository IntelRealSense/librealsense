// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <fastrtps/types/TypesBase.h>
#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include "dds-exceptions.h"

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

// Convert FastDDS Log::Entry to EasyLogging log (see ELPP_WRITE_LOG)
#define LOG_DDS_ENTRY( ENTRY, LEVEL, ... )                                                         \
    do                                                                                             \
    {                                                                                              \
        char const * filename = ( ENTRY ).context.filename;                                        \
        char const * func = ( ENTRY ).context.function;                                            \
        if( ! func )                                                                               \
            func = "n/a";                                                                          \
        if( ! filename )                                                                           \
            filename = func;                                                                       \
        el::base::Writer writer( el::Level::LEVEL, filename, ( ENTRY ).context.line, func );       \
        writer.construct( 1, "librealsense" );                                                     \
        writer << __VA_ARGS__;                                                                     \
        writer << " [DDS]";                                                                        \
        if( ( ENTRY ).context.category )                                                           \
            writer << "[" << ( ENTRY ).context.category << "]";                                    \
    }                                                                                              \
    while( false )
