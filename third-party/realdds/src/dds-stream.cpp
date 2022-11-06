// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream.h>
#include "dds-stream-impl.h"

#include <realdds/dds-exceptions.h>


namespace realdds {


dds_stream::dds_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


bool dds_stream::is_open() const
{
    return false;
}


bool dds_stream::is_streaming() const
{
    return false;
}


std::shared_ptr< dds_topic > const & dds_stream::get_topic() const
{
    DDS_THROW( runtime_error, "stream '" + name() + "' must be open to get_topic()" );
}


dds_video_stream::dds_video_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _impl( std::make_shared< dds_video_stream::impl >() )
{
}


dds_motion_stream::dds_motion_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _impl( std::make_shared< dds_motion_stream::impl >() )
{
}


}  // namespace realdds
