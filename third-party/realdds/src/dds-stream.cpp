// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream.h>
#include "dds-stream-impl.h"

#include <realdds/dds-topic.h>
#include <realdds/dds-topic-reader.h>
#include <realdds/dds-subscriber.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/dds-exceptions.h>

#include <fastdds/dds/subscriber/SampleInfo.hpp>

using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_stream::dds_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


void dds_stream::open( std::string const & topic_name, std::shared_ptr< dds_subscriber > const & subscriber )
{
    if ( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );
    if ( profiles().empty() )
        DDS_THROW( runtime_error, "stream '" + name() + "' has no profiles" );

    //Topics with same name and type can be created multiple times (multiple open() calls) without an error.
    auto topic = topics::device::image::create_topic( subscriber->get_participant(), topic_name.c_str() );

    //To support automatic streaming (without the need to handle start/stop-streaming commands) the reader is created
    //here and destroyed on close()
    _reader = std::make_shared< dds_topic_reader >( topic, subscriber );
    _reader->on_data_available( [&]() { handle_frames(); } );
    _reader->run( dds_topic_reader::qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
}


void dds_stream::close()
{
    _reader.reset();
}


void dds_stream::start_streaming( on_data_available_callback cb )
{
    if ( !is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is not open" );

    _on_data_available = cb;
    _is_streaming = true;

}


void dds_stream::handle_frames()
{
    topics::device::image frame;
    eprosima::fastdds::dds::SampleInfo info;
    while ( topics::device::image::take_next( *_reader, &frame, &info ) )
    {
        if ( !frame.is_valid() )
            continue;

        if ( _on_data_available )
            _on_data_available( std::move( frame ) );
    }
}


void dds_stream::stop_streaming()
{
    if ( !is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is not streaming" );

    _reader->on_data_available( [](){} ); //Do nothing with received frames
    _is_streaming = false;
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


dds_depth_stream::dds_depth_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_ir_stream::dds_ir_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_color_stream::dds_color_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_fisheye_stream::dds_fisheye_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_confidence_stream::dds_confidence_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_motion_stream::dds_motion_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
    , _impl( std::make_shared< dds_motion_stream::impl >() )
{
}


dds_accel_stream::dds_accel_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_gyro_stream::dds_gyro_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_pose_stream::dds_pose_stream( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}
