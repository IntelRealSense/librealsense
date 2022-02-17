// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#include <types.h>
#include "dds-enumerator.h"
#include "msg/devicesPubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipantFactory.hpp>
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/subscriber/qos/DataReaderQos.hpp>
#include <fastdds/dds/subscriber/Subscriber.hpp>

// We align the DDS topic name to ROS2 as it expect the 'rt/' prefix for the topic name
#define CONCATENATE(s1, s2) s1 ## s2
#define ROS2_PREFIX(name) CONCATENATE("rt/", name)

using namespace eprosima::fastdds::dds;

namespace librealsense {

dds_enumerator::dds_enumerator()
    : _participant( nullptr )
    , _subscriber( nullptr )
    , _topic( nullptr )
    , _reader( nullptr )
    , _type( new devicesPubSubType() )
{
}

dds_enumerator::~dds_enumerator() {}

void dds_enumerator::init()
{
    DomainParticipantQos pqos;
    pqos.name("LRS_DEVICES_CLIENT");
    _participant = DomainParticipantFactory::get_instance()->create_participant(0, pqos, &_domain_listener );

    if (_participant == nullptr)
    {
        LOG_ERROR("Error creating a DDS participant");
        throw librealsense::backend_exception( "Error creating a DDS participant", RS2_EXCEPTION_TYPE_IO );
    }

    //REGISTER THE TYPE
    _type.register_type(_participant);

    //CREATE THE SUBSCRIBER
    _subscriber = _participant->create_subscriber(SUBSCRIBER_QOS_DEFAULT, nullptr);

    if (_subscriber == nullptr)
    {
        LOG_ERROR("Error creating a DDS subscriber");
        throw librealsense::backend_exception( "Error creating a DDS subscriber", RS2_EXCEPTION_TYPE_IO );
    }

    //CREATE THE TOPIC
    // the 'rt/' prefix is places 
    _topic = _participant->create_topic(
        ROS2_PREFIX("DevicesTopic"),
        _type->getName(),
        TOPIC_QOS_DEFAULT);

    if (_topic == nullptr)
    {
        LOG_ERROR("Error creating a DDS Devices topic");
        throw librealsense::backend_exception( "Error creating a DDS Devices topic", RS2_EXCEPTION_TYPE_IO );
    }

    // CREATE THE READER
    DataReaderQos rqos = DATAREADER_QOS_DEFAULT;
    rqos.reliability().kind = RELIABLE_RELIABILITY_QOS;
    rqos.durability().kind = VOLATILE_DURABILITY_QOS;
    rqos.data_sharing().automatic();
    rqos.ownership().kind = EXCLUSIVE_OWNERSHIP_QOS;
    _reader = _subscriber->create_datareader(_topic, rqos, nullptr);

    if (_reader == nullptr)
    {
        LOG_ERROR("Error creating a DDS reader");
        throw librealsense::backend_exception( "Error creating a DDS reader", RS2_EXCEPTION_TYPE_IO );

    }
}
void dds_enumerator::DiscoveryDomainParticipantListener::on_publisher_discovery(
    DomainParticipant * participant,
    eprosima::fastrtps::rtps::WriterDiscoveryInfo && info )
{
    switch( info.status )
    {
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::DISCOVERED_WRITER:
        /* Process the case when a new publisher was found in the domain */
        std::cout << "New DataWriter publishing under topic '" << info.info.topicName()
                  << "' of type '" << info.info.typeName() << "' discovered" << std::endl;
        break;
    case eprosima::fastrtps::rtps::WriterDiscoveryInfo::REMOVED_WRITER:
        /* Process the case when a publisher was removed from the domain */
        std::cout << "New DataWriter publishing under topic '" << info.info.topicName()
                  << "' of type '" << info.info.typeName() << "' left the domain." << std::endl;
        break;
    }
}
}  // namespace librealsense
