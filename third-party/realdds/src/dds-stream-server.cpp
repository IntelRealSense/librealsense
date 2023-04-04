// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-server.h>

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/flexible/flexible-msg.h>
#include <realdds/topics/ros2/ros2imagePubSubTypes.h>
#include <realdds/dds-time.h>

#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_stream_server::dds_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : dds_stream_base( stream_name, sensor_name )
{
}


dds_stream_server::~dds_stream_server()
{
}


dds_video_stream_server::dds_video_stream_server( std::string const& stream_name, std::string const& sensor_name )
    : dds_stream_server( stream_name, sensor_name )
{
}


void dds_video_stream_server::check_profile( std::shared_ptr< dds_stream_profile > const & profile ) const
{
    super::check_profile( profile );
    if( ! std::dynamic_pointer_cast< dds_video_stream_profile >( profile ) )
        DDS_THROW( runtime_error, "profile '" + profile->to_string() + "' is not a video profile" );
}


dds_depth_stream_server::dds_depth_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_ir_stream_server::dds_ir_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_color_stream_server::dds_color_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_fisheye_stream_server::dds_fisheye_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_confidence_stream_server::dds_confidence_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_motion_stream_server::dds_motion_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : dds_stream_server( stream_name, sensor_name )
{
}


void dds_motion_stream_server::check_profile( std::shared_ptr< dds_stream_profile > const & profile ) const
{
    super::check_profile( profile );
    if( ! std::dynamic_pointer_cast< dds_motion_stream_profile >( profile ) )
        DDS_THROW( runtime_error, "profile '" + profile->to_string() + "' is not a motion profile" );
}


dds_accel_stream_server::dds_accel_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_gyro_stream_server::dds_gyro_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


dds_pose_stream_server::dds_pose_stream_server( std::string const & stream_name, std::string const & sensor_name )
    : super( stream_name, sensor_name )
{
}


void dds_video_stream_server::open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & publisher )
{
    if( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );
    if( profiles().empty() )
        DDS_THROW( runtime_error, "stream '" + name() + "' has no profiles" );

    auto topic = topics::device::image::create_topic( publisher->get_participant(), topic_name.c_str() );
    _writer = std::make_shared< dds_topic_writer >( topic, publisher );


    run_stream();
}

void dds_stream_server::run_stream()
{
    if( ! _writer )
        DDS_THROW( runtime_error, "open() wasn't called before run_stream()" );

    if( _on_readers_changed )
    {
        std::weak_ptr< dds_stream_server > weak_this(
            std::static_pointer_cast<dds_stream_server>(shared_from_this()) );
        _writer->on_publication_matched(
            [weak_this, on_readers_changed = _on_readers_changed](
                eprosima::fastdds::dds::PublicationMatchedStatus const & status )
            {
                if( auto self = weak_this.lock() )
                    try
                    {
                        LOG_DEBUG( status.current_count << " total readers on '" << self->name() << "'" );
                        on_readers_changed( self, status.current_count );
                    }
                    catch( std::exception const & e )
                    {
                        LOG_ERROR( "exception from 'on_readers_changed': " << e.what() );
                    }
            } );
    }
    
    _writer->run( dds_topic_writer::qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
}


void dds_motion_stream_server::open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & publisher )
{
    if( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );
    if( profiles().empty() )
        DDS_THROW( runtime_error, "stream '" + name() + "' has no profiles" );

    auto topic = topics::device::image::create_topic( publisher->get_participant(), topic_name.c_str() );
    _writer = std::make_shared< dds_topic_writer >( topic, publisher );


    run_stream();
}


std::shared_ptr< dds_topic > const & dds_stream_server::get_topic() const
{
    if( ! is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' must be open to get_topic()" );

    return _writer->topic();
}


void dds_stream_server::start_streaming( const image_header & header )
{
    if( ! is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' must be open before start_streaming()" );
    if( is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already streaming" );

    _image_header = header;
    _streaming = true;
}


void dds_stream_server::stop_streaming()
{
    if( ! is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is not streaming" );

    _image_header.invalidate();
    _streaming = false;
}

void dds_stream_server::close()
{
    _writer.reset();
}

void dds_stream_server::publish( const uint8_t * data, size_t size, unsigned long long id )
{
    if( ! is_streaming() )
        DDS_THROW( runtime_error, "stream '" + name() + "' cannot publish before start_streaming()" );

    // LOG_DEBUG( "publishing a DDS video frame for topic: " << _writer->topic()->get()->get_name() );
    sensor_msgs::msg::Image raw_image;
    raw_image.header().frame_id() = std::to_string( id );
    auto const now = realdds::now();
    raw_image.header().stamp().sec() = now.seconds;
    raw_image.header().stamp().nanosec() = now.nanosec;
    raw_image.encoding() = _image_header.format.to_string();
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.step() = uint32_t( size / _image_header.height );
    raw_image.is_bigendian() = false;
    raw_image.data().assign( data, data + size );
    LOG_DEBUG( "publishing '" << name() << "' frame " << std::dec << id << " " << raw_image.encoding() );

    DDS_API_CALL( _writer->get()->write( &raw_image ) );
}
