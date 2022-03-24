// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2022 Intel Corporation. All Rights Reserved.

#pragma once

#include <iostream>
#include <unordered_map>

#include "../../src/dds/msg/devicesPubSubTypes.h"
#include <fastdds/dds/domain/DomainParticipant.hpp>
#include <fastdds/dds/publisher/DataWriterListener.hpp>
#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API


class dds_server
{
public:
    dds_server();
    ~dds_server();

    void init();

private:

    class dds_serverListener : public eprosima::fastdds::dds::DataWriterListener
    {
    public:
        // If we'll want to know about readers of our topics
        void on_publication_matched(
            eprosima::fastdds::dds::DataWriter * writer,
            const eprosima::fastdds::dds::PublicationMatchedStatus & info ) override;

        int _matched = 0;
    };

    struct device_info
    {
        rs2::device device;
        eprosima::fastdds::dds::DataWriter* data_writer;
        std::shared_ptr<dds_serverListener> listener;
    };

    eprosima::fastdds::dds::DomainParticipant * _participant;
    eprosima::fastdds::dds::Publisher * _publisher;
    eprosima::fastdds::dds::Topic * _topic;
    devices _devicesTopic;
    eprosima::fastdds::dds::TypeSupport _type_support_ptr;
    std::unordered_map<std::string, device_info> _devices_writers;
    rs2::context _ctx;
};  // class dds_server
