// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <stdexcept>
#include <string>


// E.g.:
//     DDS_THROW( runtime_error, "what happened" );
//
#define DDS_THROW( ERR_TYPE, WHAT )                                                                 \
    throw realdds::dds_ ## ERR_TYPE ( WHAT )


namespace realdds {


class dds_runtime_error : public std::runtime_error
{
public:
    dds_runtime_error( std::string const& str )
        : std::runtime_error( str )
    {
        LOG_ERROR( "throwing: " << str);
    }

    dds_runtime_error( char const* lpsz )
        : std::runtime_error( lpsz )
    {
        LOG_ERROR( "throwing: " << lpsz );
    }
};

class dds_length_error : public std::length_error
{
public:
    dds_length_error( std::string const & str )
        : std::length_error( str )
    {
        LOG_ERROR( "throwing: " << str );
    }

    dds_length_error( char const * lpsz )
        : std::length_error( lpsz )
    {
        LOG_ERROR( "throwing: " << lpsz );
    }
};


}  // namespace realdds
