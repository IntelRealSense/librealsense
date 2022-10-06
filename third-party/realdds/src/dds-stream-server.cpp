// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-server.h>

#include <realdds/dds-topic-writer.h>
#include <realdds/dds-topic.h>
#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/image/image-msg.h>
#include <realdds/topics/image/imagePubSubTypes.h>

#include <fastdds/dds/topic/Topic.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>


using namespace eprosima::fastdds::dds;
using namespace realdds;


dds_stream_server::dds_stream_server( std::shared_ptr< dds_topic_writer > const & writer )
    : _writer( writer )
{
}


dds_stream_server::~dds_stream_server()
{
}


std::shared_ptr< dds_topic > const& dds_stream_server::get_topic() const
{
    return _writer->topic();
}


void dds_stream_server::start_streaming( const image_header & header )
{
    if( ! header.is_valid() )
        DDS_THROW( runtime_error, "cannot start_streaming() with an invalid header" );
    _image_header = header;
}


void dds_stream_server::publish_image( const uint8_t * data, size_t size )
{
    if( ! _image_header.is_valid() )
        DDS_THROW( runtime_error, "cannot publish_image() before you start_streaming()" );

    LOG_DEBUG( "publishing a DDS video frame for topic: " << _writer->topic()->get()->get_name() );
    topics::raw::device::image raw_image;
    raw_image.size() = static_cast< uint32_t >( size );
    raw_image.format() = _image_header.format;
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.raw_data().assign( data, data + size );

    DDS_API_CALL( _writer->get()->write( &raw_image ) );
}
