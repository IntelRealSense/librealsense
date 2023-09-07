// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include <fastdds/rtps/common/Guid.h>


namespace realdds {


constexpr auto & unknown_guid = eprosima::fastrtps::rtps::c_Guid_Unknown;


// Custom GUID printer: attempts a more succinct representation
// If a base_prefix is provided, will try to minimize a common denominator (vendor, host, etc.) -- you can use your
// participant's guid if you want to shorten
//
std::string print( dds_guid const & guid,
                   dds_guid_prefix const & base_prefix = unknown_guid.guidPrefix,
                   bool readable_name = true );

// Same as above, without a base prefix
inline std::string print( dds_guid const & guid, bool readable_name )
{
    return print( guid, unknown_guid.guidPrefix, readable_name );
}

// Same as above, with a guid base for flexibility
inline std::string print( dds_guid const & guid, dds_guid const & base_guid, bool readable_name = true )
{
    return print( guid, base_guid.guidPrefix, readable_name );
}


}  // namespace realdds
