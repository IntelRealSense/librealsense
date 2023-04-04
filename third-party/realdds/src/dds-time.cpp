// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-time.h>
#include <realdds/dds-utilities.h>


namespace realdds {


std::string time_to_string( dds_time const & t )
{
    std::string nsec = std::to_string( t.nanosec );
    if( t.nanosec )
    {
        nsec.insert( 0, 9 - nsec.length(), '0' );
        while( nsec.length() > 1 && nsec.back() == '0' )
            nsec.pop_back();
    }
    return std::to_string( t.seconds ) + '.' + nsec;
}


}  // namespace realdds
