// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-server.h>

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-publisher.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/image/imagePubSubTypes.h>

#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_stream_server::dds_stream_server( std::string const & name,
                                      dds_stream_profiles const & profiles,
                                      int default_profile_index )
    : _name( name )
    , _profiles( profiles )
    , _default_profile_index( default_profile_index )
{
    if( profiles.empty() )
        DDS_THROW( runtime_error, "at least one profile is required in stream '" + name + "'" );
}


dds_stream_server::~dds_stream_server()
{
}


dds_video_stream_server::dds_video_stream_server( std::string const & name,
                                                  dds_stream_profiles const & profiles,
                                                  int default_profile_index )
    : dds_stream_server( name, profiles, default_profile_index )
{
    for( auto & profile : profiles )
        if( ! std::dynamic_pointer_cast< dds_video_stream_profile >( profile ) )
            DDS_THROW( runtime_error, "profile '" + profile->to_string() + "' is not a video profile" );
}


dds_motion_stream_server::dds_motion_stream_server( std::string const & name,
                                                    dds_stream_profiles const & profiles,
                                                    int default_profile_index )
    : dds_stream_server( name, profiles, default_profile_index )
{
    for( auto & profile : profiles )
        if( ! std::dynamic_pointer_cast< dds_motion_stream_profile >( profile ) )
            DDS_THROW( runtime_error, "profile '" + profile->to_string() + "' is not a motion profile" );
}


void dds_video_stream_server::open( std::string const & topic_name, std::shared_ptr< dds_publisher > const & publisher )
{
    if( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );

    auto topic = topics::device::image::create_topic( publisher->get_participant(), topic_name.c_str() );

    _writer = std::make_shared< dds_topic_writer >( topic, publisher );
    _writer->run( dds_topic_writer::writer_qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
}


void dds_motion_stream_server::open( std::string const & topic_name,
                                     std::shared_ptr< dds_publisher > const & publisher )
{
    if( is_open() )
        DDS_THROW( runtime_error, "stream '" + name() + "' is already open" );

    auto topic = topics::device::image::create_topic( publisher->get_participant(), topic_name.c_str() );

    _writer = std::make_shared< dds_topic_writer >( topic, publisher );
    _writer->run( dds_topic_writer::writer_qos( BEST_EFFORT_RELIABILITY_QOS ) );  // no retries
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
    if( ! header.is_valid() )
        DDS_THROW( runtime_error, "stream '" + name() + "' cannot start_streaming() with an invalid header" );
    _image_header = header;
}


void dds_stream_server::publish_image( const uint8_t * data, size_t size )
{
    if( ! _image_header.is_valid() )
        DDS_THROW( runtime_error, "stream '" + name() + "' cannot publish_image() before start_streaming()" );

    LOG_DEBUG( "publishing a DDS video frame for topic: " << _writer->topic()->get()->get_name() );
    topics::raw::device::image raw_image;
    raw_image.size() = static_cast< uint32_t >( size );
    raw_image.format() = _image_header.format;
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.raw_data().assign( data, data + size );

    DDS_API_CALL( _writer->get()->write( &raw_image ) );
}
