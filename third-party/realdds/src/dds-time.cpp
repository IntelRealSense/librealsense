// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2023 Intel Corporation. All Rights Reserved.

#include <realdds/dds-time.h>
#include <realdds/dds-utilities.h>


namespace realdds {


std::ostream & operator<<( std::ostream & os, time_to_string const & t )
{
    if( t._seconds == eprosima::fastrtps::c_TimeInvalid.seconds
        && t._nanosec == eprosima::fastrtps::c_TimeInvalid.nanosec )
        return os << "INVALID";

    os << t._seconds << '.';

    if( ! t._nanosec )
        return os << '0';

    std::string nsec = std::to_string( t._nanosec );
    // DDS spec 2.3.2: "the nanosec field must verify 0 <= nanosec < 1000000000"
    nsec.insert( 0, 9 - nsec.length(), '0' );  // will throw if more than 9 digits!
    while( nsec.length() > 1 && nsec.back() == '0' )
        nsec.pop_back();
    return os << nsec;
}


}  // namespace realdds
