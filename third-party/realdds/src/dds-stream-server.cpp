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
#include <realdds/topics/image/imagePubSubTypes.h>

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


void dds_stream_server::open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & publisher )
{
    if( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );
    if( profiles().empty() )
        DDS_THROW( runtime_error, "stream '" + name() + "' has no profiles" );

    auto image_topic = topics::device::image::create_topic( publisher->get_participant(), topic_name.c_str() );

    _image_writer = std::make_shared< dds_topic_writer >( image_topic, publisher );
    _image_writer->run( dds_topic_writer::qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries

    auto md_topic = topics::flexible_msg::create_topic( publisher->get_participant(), ( topic_name + "/md" ).c_str() );

    _metadata_writer = std::make_shared< dds_topic_writer >( md_topic, publisher );
    _metadata_writer->run( dds_topic_writer::qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
}


std::shared_ptr< dds_topic > const & dds_stream_server::get_topic() const
{
    if( ! is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' must be open to get_topic()" );

    return _image_writer->topic();
}


void dds_stream_server::start_streaming( const image_header & header )
{
    if( ! is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' must be open before start_streaming()" );
    if( ! header.is_valid() )
        DDS_THROW( runtime_error, "stream '" + name() + "' cannot start_streaming() with an invalid header" );
    _image_header = header;
}


void dds_stream_server::stop_streaming()
{
    _image_header.invalidate();
}


void dds_stream_server::publish_image( const uint8_t * data, size_t size, const nlohmann::json & metadata )
{
    if( ! _image_header.is_valid() )
        DDS_THROW( runtime_error, "stream '" + name() + "' cannot publish_image() before start_streaming()" );

    LOG_DEBUG( "publishing a DDS video frame for topic: " << _image_writer->topic()->get()->get_name() );
    topics::raw::device::image raw_image;
    raw_image.size() = static_cast< uint32_t >( size );
    raw_image.format() = _image_header.format;
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.raw_data().assign( data, data + size );
    raw_image.frame_id() = rsutils::json::get< std::string >( metadata, "frame_id" );

    DDS_API_CALL( _image_writer->get()->write( &raw_image ) );

    topics::flexible_msg md_message( metadata );
    md_message.write_to( *_metadata_writer );
}
