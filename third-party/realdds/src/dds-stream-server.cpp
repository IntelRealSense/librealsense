// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <realdds/dds-stream-server.h>

#include <realdds/dds-participant.h>
#include <realdds/dds-utilities.h>
#include <realdds/topics/dds-topics.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <librealsense2/utilities/concurrency/concurrency.h>

#include <iostream>

using namespace eprosima::fastdds::dds;
using namespace realdds;

dds_stream_server::dds_stream_server( std::shared_ptr< dds_participant > const & participant,
                                      eprosima::fastdds::dds::Publisher * publisher,
                                      const std::string & topic_root,
                                      const std::string & stream_name )
    : _participant( participant )
    , _publisher( publisher )
    , _topic( nullptr )
    , _data_writer( nullptr )
{
    std::string topic_name = topics::device::image::construct_topic_name( topic_root, stream_name );

    eprosima::fastdds::dds::TypeSupport topic_type( new topics::device::image::type );

    DDS_API_CALL( _participant->get()->register_type( topic_type ) );
    _topic = DDS_API_CALL( _participant->get()->create_topic( topic_name, topic_type->getName(), TOPIC_QOS_DEFAULT ) );

    // TODO:: Maybe we want to open a writer only when the stream is requested?
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
    wqos.data_sharing().off();
    wqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    wqos.durability().kind = VOLATILE_DURABILITY_QOS;
    wqos.publish_mode().kind = SYNCHRONOUS_PUBLISH_MODE;
    // Our message has dynamic size (unbounded) so we need a memory policy that supports it
    wqos.endpoint().history_memory_policy = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

    _data_writer = DDS_API_CALL( _publisher->create_datawriter( _topic, wqos ) );
}


dds_stream_server::~dds_stream_server()
{
    if( nullptr != _data_writer )
    {
        DDS_API_CALL_NO_THROW( _publisher->delete_datawriter( _data_writer ) );
    }

    if( nullptr != _topic )
    {
        DDS_API_CALL_NO_THROW( _participant->get()->delete_topic( _topic ) );
    }
}


void dds_stream_server::publish_image( const uint8_t * data, size_t size )
{
    LOG_DEBUG( "publishing a DDS video frame for topic: " << this->_data_writer->get_topic()->get_name() );
    topics::raw::device::image raw_image;
    raw_image.size() = static_cast< uint32_t >( size );
    raw_image.format() = _image_header.format;
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.raw_data().assign( data, data + size );

    DDS_API_CALL( _data_writer->write( &raw_image ) );
}
