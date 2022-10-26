// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <vector>

namespace realdds {


class dds_video_stream::impl
{
public:
    impl( const std::string & group_name )
        : _group_name( group_name )
    {
    }

    std::string _group_name;

    std::vector< std::pair< dds_video_stream_profile, bool > > _profiles; //Pair of profile and is default for stream
};

class dds_motion_stream::impl
{
public:
    impl( const std::string & group_name )
        : _group_name( group_name )
    {
    }

    std::string _group_name;

    std::vector< std::pair< dds_motion_stream_profile, bool > > _profiles; //Pair of profile and is default for stream
};


}  // namespace realdds
