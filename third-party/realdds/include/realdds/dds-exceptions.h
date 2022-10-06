// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <librealsense2/utilities/easylogging/easyloggingpp.h>
#include <stdexcept>
#include <string>


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


}  // namespace realdds
