// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

namespace realdds {


class dds_video_stream::impl
{
public:
    impl( int8_t type, const std::string & group_name )
        : _type( type )
        , _group_name( group_name )
    {
    }

    int8_t _type;
    std::string _group_name;

    std::vector< std::pair< dds_video_stream::profile, bool > > _profiles; //Pair of profile and is default for stream
};

class dds_motion_stream::impl
{
public:
    impl( int8_t type, const std::string & group_name )
        : _type( type )
        , _group_name( group_name )
    {
    }

    int8_t _type;
    std::string _group_name;

    std::vector< std::pair< dds_motion_stream::profile, bool > > _profiles; //Pair of profile and is default for stream
};


}  // namespace realdds
