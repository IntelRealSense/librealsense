// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022-4 Intel Corporation. All Rights Reserved.
#pragma once

#include <rsutils/easylogging/easyloggingpp.h>
#include <stdexcept>


// E.g.:
//     DDS_THROW( runtime_error, "what happened" );
//
#define DDS_THROW( ERR_TYPE, WHAT )                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        std::ostringstream os__;                                                                                       \
        os__ << WHAT;                                                                                                  \
        auto log_message__ = os__.str();                                                                               \
        LOG_ERROR_STR( "throwing: " << log_message__ );                                                                \
        throw realdds::dds_##ERR_TYPE( std::move( log_message__ ) );                                                   \
    }                                                                                                                  \
    while( 0 )


namespace realdds {


class dds_runtime_error : public std::runtime_error
{
public:
    using std::runtime_error::runtime_error;
};


}  // namespace realdds
