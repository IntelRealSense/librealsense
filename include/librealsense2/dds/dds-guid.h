// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include <fastdds/rtps/common/Guid.h>
#include <sstream>

namespace librealsense {
namespace dds {


// Custom GUID printer: attempts a more succinct representation
// If a base_prefix is provided, will try to minimize a common denominator (vendor, host, etc.) -- you can use your
// participant's guid if you want to shorten
//
inline std::string print( dds_guid const & guid,
                          dds_guid_prefix const & base_prefix = eprosima::fastrtps::rtps::c_Guid_Unknown.guidPrefix )
{
    std::ostringstream output;
    if( guid != eprosima::fastrtps::rtps::c_Guid_Unknown )
    {
        output << std::hex;
        char old_fill = output.fill( '0' );
        size_t i = 0;
        if( base_prefix != eprosima::fastrtps::rtps::c_Guid_Unknown.guidPrefix )
        {
            while( i < base_prefix.size && guid.guidPrefix.value[i] == base_prefix.value[i] )
                ++i;
        }
        if( i < guid.guidPrefix.size )
        {
            for( ; i < guid.guidPrefix.size; ++i )
                output << std::setw( 2 ) << +guid.guidPrefix.value[i];  // << ".";

            output << '|';  // between prefix and entity
        }
        for( uint8_t i = 0; i < guid.entityId.size; ++i )
            output << std::setw( 2 ) << +guid.entityId.value[i];

        output.fill( old_fill );
        output << std::dec;
    }
    else
    {
        output << "|GUID UNKNOWN|";
    }
    return output.str();
}

// Same as above, with a guid base for flexibility
inline std::string print( dds_guid const & guid, dds_guid const & base_guid )
{
    return print( guid, base_guid.guidPrefix );
}


}  // namespace dds
}  // namespace librealsense
