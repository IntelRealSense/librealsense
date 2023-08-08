// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <rsutils/easylogging/easyloggingpp.h>
#include <stdexcept>
#include <rsutils/string/from.h>


// E.g.:
//     DDS_THROW( runtime_error, "what happened" );
//
#define DDS_THROW( ERR_TYPE, WHAT )                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        LOG_ERROR( "throwing: " << WHAT );                                                                             \
        throw realdds::dds_##ERR_TYPE( rsutils::string::from() << WHAT );                                              \
    }                                                                                                                  \
    while( 0 )


namespace realdds {


class dds_runtime_error : public std::runtime_error
{
public:
    dds_runtime_error( std::string const& str )
        : std::runtime_error( str )
    {
    }

    dds_runtime_error( char const* lpsz )
        : std::runtime_error( lpsz )
    {
    }
};


}  // namespace realdds
