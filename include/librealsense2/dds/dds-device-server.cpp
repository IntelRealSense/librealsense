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
    , _topic( nullptr )
    , _topic_root( topic_root )
    , _topic_type_ptr(std::make_shared<eprosima::fastdds::dds::TypeSupport>(new librealsense::dds::topics::image::type))
    , _init_ok( false )
{
    std::cout << "DDS device server for device root: " << _topic_root << " created" << std::endl;
}

dds_device_server::~dds_device_server()
{
    std::cout << "DDS device server for device root: " << _topic_root << " deleted" << std::endl;

    // Release resources
    if( nullptr != _participant )
    {
        if( nullptr != _topic )
        {
            _participant->delete_topic( _topic );
        }

        if( nullptr != _publisher )
        {
            _participant->delete_publisher( _publisher );
        }
    }
}

bool dds_device_server::create_dds_publisher( const std::string& topic_name )
{
    _topic_type_ptr->register_type( _participant );
    _publisher = _participant->create_publisher( PUBLISHER_QOS_DEFAULT, nullptr );
    _topic = _participant->create_topic( topic_name,
                                         (*_topic_type_ptr)->getName(),
                                         TOPIC_QOS_DEFAULT );

    return ( _topic != nullptr && _publisher != nullptr );
}

void dds_device_server::publish_dds_video_frame( const std::string& topic_name, uint8_t* frame )
{
    if( !_init_ok )
        _init_ok = create_dds_publisher( topic_name );

    if( _init_ok )
    {
        LOG_DEBUG( "publishing a DDS video frame for topic: " << topic_name );
        //librealsense::dds::topics::image image;
        // prepare_image( image );
        // publish_frame( image );
    }

}
