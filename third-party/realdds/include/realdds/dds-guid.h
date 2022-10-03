// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include "dds-defines.h"
#include <fastdds/rtps/common/Guid.h>


namespace realdds {


// Custom GUID printer: attempts a more succinct representation
// If a base_prefix is provided, will try to minimize a common denominator (vendor, host, etc.) -- you can use your
// participant's guid if you want to shorten
//
std::string print( dds_guid const & guid,
                   dds_guid_prefix const & base_prefix = eprosima::fastrtps::rtps::c_Guid_Unknown.guidPrefix );

// Same as above, with a guid base for flexibility
inline std::string print( dds_guid const & guid, dds_guid const & base_guid )
{
    return print( guid, base_guid.guidPrefix );
}


}  // namespace realdds
