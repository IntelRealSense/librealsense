// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <iostream>

#include "dds-device-server.h"
#include "dds-participant.h"

#include <librealsense2/utilities/easylogging/easyloggingpp.h>

#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/Publisher.hpp>
#include <fastdds/dds/publisher/DataWriter.hpp>
#include <fastdds/dds/publisher/qos/PublisherQos.hpp>
#include <librealsense2/dds/topics/dds-topics.h>

using namespace eprosima::fastdds::dds;
using namespace librealsense::dds;


dds_device_server::dds_device_server( dds_participant & participant, const std::string &topic_root )
    : _participant( participant.get() )
    , _publisher(nullptr)
    , _topic_root( topic_root )
{
    LOG_DEBUG( "DDS device server for device root: " << _topic_root << " created");
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

    stream_name_to_server.clear();
    LOG_DEBUG( "DDS device server for device root: " << _topic_root << " deleted" );
}

bool dds_device_server::init( const std::vector<std::string> &supported_streams_names )
{
    if( is_valid() )
    {
        throw std::runtime_error( "publisher is already initialized; cannot init a publisher for'" + _topic_root);
    }

    if( !create_dds_publisher() )
        return false;
    
    for( auto stream_name : supported_streams_names )
    {
        stream_name_to_server[stream_name] = std::make_shared< dds_stream_server >( _participant,
                                                                                    _publisher,
                                                                                    _topic_root,
                                                                                    stream_name );
    }

    return true;
}

bool dds_device_server::create_dds_publisher( )
{
    _publisher = _participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr );
    return ( _publisher != nullptr );
}

void dds_device_server::dds_stream_server::publish_video_frame( uint8_t * frame, int size )
{
    LOG_DEBUG( "publishing a DDS video frame for topic: " << _topic_name );
    librealsense::dds::topics::raw::image raw_image;
    raw_image.size() = size;
    raw_image.format() = _image_header.format;
    raw_image.height() = _image_header.height;
    raw_image.width() = _image_header.width;
    raw_image.raw_data().assign( frame, frame + size );

    _data_writer->write( &raw_image );
}

dds_device_server::dds_stream_server::dds_stream_server( eprosima::fastdds::dds::DomainParticipant * participant, eprosima::fastdds::dds::Publisher * publisher,  const std::string& topic_root, const std::string& stream_name )
    : _participant( participant )
    , _publisher( publisher )
    , _topic( nullptr )
    , _topic_type_ptr( std::make_shared< eprosima::fastdds::dds::TypeSupport >( new librealsense::dds::topics::image::type ) )
    , _data_writer( nullptr )
{
    _topic_name = librealsense::dds::topics::image::construct_stream_topic_name( topic_root, stream_name );
    _topic_type_ptr->register_type( _participant );
    _topic = _participant->create_topic( _topic_name,
                                         ( *_topic_type_ptr )->getName(),
                                         TOPIC_QOS_DEFAULT );

    // TODO:: Maybe we want to open a writer only when the stream is requested?
    DataWriterQos wqos = DATAWRITER_QOS_DEFAULT;  
    wqos.data_sharing().off();
    wqos.reliability().kind = BEST_EFFORT_RELIABILITY_QOS;
    wqos.durability().kind = VOLATILE_DURABILITY_QOS;
    wqos.publish_mode().kind = SYNCHRONOUS_PUBLISH_MODE;

    _data_writer = _publisher->create_datawriter( _topic, wqos );

    if( ! _topic || ! _data_writer )
        throw std::runtime_error(
            "Error in creating DDS resources for video stream server of topic: " + _topic_name );
}

dds_device_server::dds_stream_server::~dds_stream_server()
{
    if( nullptr != _participant )
    {
        if( nullptr != _topic )
        {
            _participant->delete_topic( _topic );
        }

        if( nullptr != _data_writer )
        {
            _publisher->delete_datawriter( _data_writer );
        }
    }
}
