// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <types.h>
#include "dds-device-watcher.h"
#include "msg/devicesPubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

// We align the DDS topic name to ROS2 as it expect the 'rt/' prefix for the topic name
#define ROS2_PREFIX(name) std::string("rt/").append(name)

using namespace eprosima::fastdds::dds;

namespace librealsense {

dds_device_watcher::dds_device_watcher()
    : _participant( nullptr )
    , _subscriber( nullptr )
    , _topic( nullptr )
    , _reader( nullptr )
    , _type_ptr( new devicesPubSubType() )
{
}

dds_device_watcher::~dds_device_watcher() 
{
    if (_reader != nullptr)
    {
        _subscriber->delete_datareader(_reader);
    }
    if (_subscriber != nullptr)
    {
        _participant->delete_subscriber(_subscriber);
    }
    if (_topic != nullptr)
    {
        _participant->delete_topic(_topic);
    }
    DomainParticipantFactory::get_instance()->delete_participant(_participant);
}

void dds_device_watcher::init()
{
    DomainParticipantQos pqos;
    pqos.name("LRS_DEVICES_CLIENT");

    // Indicates for how much time should a remote DomainParticipant consider the local
    // DomainParticipant to be alive.
    pqos.wire_protocol().builtin.discovery_config.leaseDuration = { 10, 0 };  //[sec]

    _participant = DomainParticipantFactory::get_instance()->create_participant(0, pqos, &_domain_listener );

    if (_participant == nullptr)
    {
        LOG_ERROR("Error creating a DDS participant");
        throw librealsense::backend_exception( "Error creating a DDS participant", RS2_EXCEPTION_TYPE_IO );
    }

    //REGISTER THE TYPE
    _type_ptr.register_type(_participant);

    //CREATE THE SUBSCRIBER
    _subscriber = _participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (_subscriber == nullptr)
    {
        LOG_ERROR("Error creating a DDS subscriber");
        throw librealsense::backend_exception( "Error creating a DDS subscriber", RS2_EXCEPTION_TYPE_IO );
    }

    // CREATE THE TOPIC
    _topic = _participant->create_topic(
        ROS2_PREFIX("DevicesTopic"),
        _type_ptr->getName(),
        TOPIC_QOS_DEFAULT);

    if (_topic == nullptr)
    {
        LOG_ERROR("Error creating a DDS Devices topic");
        throw librealsense::backend_exception( "Error creating a DDS Devices topic", RS2_EXCEPTION_TYPE_IO );
    }

    // CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;

    // The 'depth' parameter of the History defines how many samples are stored before starting to
    // overwrite them with newer ones.
    rqos.history().kind = KEEP_LAST_HISTORY_QOS;
    rqos.history().depth = 10;

    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS; // We don't want to miss connection/disconnection events KOjk
    rqos.durability().kind = VOLATILE_DURABILITY_QOS;   // The Subscriber receives samples from the moment it comes online, not before
    rqos.data_sharing().automatic();                    // If possible, use shared memory
    rqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
    _reader = _subscriber->create_datareader(_topic, rqos, nullptr);

    if (_reader == nullptr)
    {
        LOG_ERROR("Error creating a DDS reader");
        throw librealsense::backend_exception( "Error creating a DDS reader", RS2_EXCEPTION_TYPE_IO );

    }


    LOG_DEBUG("DDS enumerator initialized successfully");
}
void dds_device_watcher::DiscoveryDomainParticipantListener::on_publisher_discovery(
    DomainParticipant * participant,
    eprosima::fastrtps::rtps::WriterDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
        /* Process the case when a new publisher was found in the domain */
        LOG_DEBUG("New DataWriter publishing under topic '" << info.info.topicName()
                  << "' of type '" << info.info.typeName() << "' discovered");
        break;
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
        /* Process the case when a publisher was removed from the domain */
        LOG_DEBUG("New DataWriter publishing under topic '" << info.info.topicName()
                  << "' of type '" << info.info.typeName() << "' left the domain.");
        break;
    }
}
}  // namespace librealsense
