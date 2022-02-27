// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once
#include <iostream>

#include <fastdds/dds/domain/DomainParticipantListener.hpp>

namespace librealsense {

//struct dds_device_info 
//{
//    std::string device_name;
//};

class dds_device_watcher
{
public:
    dds_device_watcher();
    ~dds_device_watcher();
    void init(); // May throw

private:

    class DiscoveryDomainParticipantListener
        : public eprosima::fastdds::dds::DomainParticipantListener
    {
        virtual void on_publisher_discovery( eprosima::fastdds::dds::DomainParticipant * participant,
                                             eprosima::fastrtps::rtps::WriterDiscoveryInfo && info ) override;
    } _domain_listener;

    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Subscriber* _subscriber;
    eprosima::fastdds::dds::Topic* _topic;
    eprosima::fastdds::dds::DataReader* _reader;
    eprosima::fastdds::dds::TypeSupport _type_ptr;
};
}  // namespace librealsense
