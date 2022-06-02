// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "dds-device-server.h"
#include "dds-participant.h"
#include "dds-utilities.h"

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <librealsense2/dds/topics/dds-topics.h>

using namespace eprosima::fastdds::dds;
using namespace librealsense::dds;


class dds_device_server::dds_stream_server
{
public:
    dds_stream_server( eprosima::fastdds::dds::DomainParticipant * participant,
                       eprosima::fastdds::dds::Publisher * publisher,
                       const std::string & topic_root,
                       const std::string & stream_name )
        : _participant( participant )
        , _publisher( publisher )
        , _topic( nullptr )
        , _data_writer( nullptr )
    {
        _topic_name = librealsense::dds::topics::image::construct_stream_topic_name( topic_root,
                                                                                     stream_name );

        eprosima::fastdds::dds::TypeSupport topic_type( new librealsense::dds::topics::image::type );

        DDS_API_CALL( _participant->register_type( topic_type ) );
        _topic = DDS_API_CALL( _participant->create_topic( _topic_name, topic_type->getName(), TOPIC_QOS_DEFAULT ) );

        // TODO:: Maybe we want to open a writer only when the stream is requested?
        DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;
        wqos.data_sharing().off();
        wqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
        wqos.durability().kind = VOLATILE_DURABILITY_QOS;
        wqos.publish_mode().kind = SYNCHRONOUS_PUBLISH_MODE;
        // Our message has dynamic size (unbounded) so we need a memory policy that supports it
        wqos.endpoint().history_memory_policy
            = eprosima::fastrtps::rtps::PREALLOCATED_WITH_REALLOC_MEMORY_MODE;

        _data_writer = DDS_API_CALL( _publisher->create_datawriter( _topic, wqos ) );
    }
    ~dds_stream_server()
    {
        if( nullptr != _data_writer )
        {
            DDS_API_CALL_NO_THROW( _publisher->delete_datawriter( _data_writer ) );
        }

        if( nullptr != _topic )
        {
            DDS_API_CALL_NO_THROW( _participant->delete_topic( _topic ) );
        }
    }


    void publish_image( const uint8_t * data, size_t size )
    {
        LOG_DEBUG( "publishing a DDS video frame for topic: " << _topic_name );
        librealsense::dds::topics::raw::image raw_image;
        raw_image.size() = static_cast< uint32_t >( size );
        raw_image.format() = _image_header.format;
        raw_image.height() = _image_header.height;
        raw_image.width() = _image_header.width;
        raw_image.raw_data().assign( data, data + size );

        DDS_API_CALL( _data_writer->write( &raw_image ) );
    }
    void set_image_header( const image_header & header ) { _image_header = header; }

private:
    std::string _topic_name;
    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    eprosima::fastdds::dds::DataWriter * _data_writer;
    image_header _image_header;
};

dds_device_server::dds_device_server( dds_participant & participant,
                                      const std::string & topic_root )
    : _participant( participant.get() )
    , _publisher( nullptr )
    , _topic_root( topic_root )
{
    LOG_DEBUG( "DDS device server for device topic root: " << _topic_root << " created" );
}

dds_device_server::~dds_device_server()
{
    // Release resources
    if( nullptr != _participant )
    {
        if( nullptr != _publisher )
        {
            _participant->delete_publisher( _publisher );
        }
    }

    _stream_name_to_server.clear();
    LOG_DEBUG( "DDS device server for device topic root: " << _topic_root << " deleted" );
}

void dds_device_server::set_image_header( const std::string & stream_name,
                                          const image_header & header )
{
    _stream_name_to_server.at( stream_name )->set_image_header( header );
}

void dds_device_server::publish_image( const std::string & stream_name, const uint8_t * data, size_t size )
{
    if( !is_valid() )
    {
        throw std::runtime_error( "Cannot publish '" + stream_name + "' frame for '" + _topic_root
                                  + "', DDS device server in uninitialized" );
    }

    _stream_name_to_server.at( stream_name )->publish_image( data, size );
}

void dds_device_server::init( const std::vector<std::string> &supported_streams_names )
{
    if( is_valid() )
    {
        throw std::runtime_error( "device server '" + _topic_root + "' is already initialized" );
    }

    _publisher = DDS_API_CALL(_participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr ));

    for( auto stream_name : supported_streams_names )
    {
        _stream_name_to_server[stream_name] = std::make_shared< dds_stream_server >( _participant,
                                                                                    _publisher,
                                                                                    _topic_root,
                                                                                    stream_name );
    }
}

